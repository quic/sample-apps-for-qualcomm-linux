# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------

import sys
import os
sys.path.append(".")
sys.path.append("python")
os.environ['HF_HUB_DISABLE_SYMLINKS_WARNING'] = "1"  # Disable 'cache-system uses symlinks' warning.
os.environ['HF_ENDPOINT'] = "https://hf-api.gitee.com"

import utils.install as install
import time
from PIL import Image
import numpy as np
import torch
from transformers import CLIPTokenizer
from diffusers import DPMSolverMultistepScheduler
from diffusers.models.embeddings import get_timestep_embedding, TimestepEmbedding
import argparse

from qai_appbuilder import (QNNContext, Runtime, LogLevel, ProfilingLevel, PerfProfile, QNNConfig, timer)

from flask import Flask, request, jsonify, send_file
import datetime

print(">>> Running server from:", __file__)

app = Flask(__name__)

####################################################################
# Model / Asset IDs and paths
MODEL_NAME                  = "stable_diffusion_v1_5"
TEXT_ENCODER_MODEL_NAME     = "text_encoder.bin"
UNET_MODEL_NAME             = "unet.bin"
VAE_DECODER_MODEL_NAME      = "vae.bin"


# NOTE: SD 1.5 uses CLIP ViT-L/14 for text encoder.
TOKENIZER_MODEL_NAME        = "openai/clip-vit-large-patch14"
TOKENIZER_HELP_URL          = "https://github.com/quic/ai-engine-direct-helper/blob/main/samples/python/" + MODEL_NAME + "/README.md#clip-vit-l14-model"
MODEL_HELP_URL = "https://github.com/quic/ai-engine-direct-helper/tree/main/samples/python/" + MODEL_NAME + "#" + MODEL_NAME + "-qnn-models"

####################################################################

execution_ws = os.getcwd()
qnn_dir = os.path.join(execution_ws, "qai_libs")

if "python" not in execution_ws:
    execution_ws = os.path.join(execution_ws, "python")

if MODEL_NAME not in execution_ws:
    execution_ws = os.path.join(execution_ws, MODEL_NAME)

# Model paths.
model_dir = os.path.join(execution_ws, "models")
sd_dir = model_dir
tokenizer_dir = os.path.join(model_dir, "tokenizer")
time_embedding_dir = os.path.join(model_dir, "time-embedding")#not used

text_encoder_model_path = os.path.join(sd_dir, TEXT_ENCODER_MODEL_NAME)
unet_model_path = os.path.join(sd_dir, UNET_MODEL_NAME)
vae_decoder_model_path = os.path.join(sd_dir, VAE_DECODER_MODEL_NAME)

tokenizer = None
scheduler = None
tokenizer_max_length = 77   # must be 77 for CLIP

# model objects.
text_encoder = None
unet = None
vae_decoder = None

# Any user defined prompt
user_prompt = ""
uncond_prompt = ""
user_seed = np.int64(0)
user_step = 20              # User defined step value, any integer value in {20, 30, 50}
user_text_guidance = 7.5    # User define text guidance, any float value in [5.0, 15.0]

####################################################################
# Helpers

MAX_TOKENS = 77  # keep consistent with tokenizer_max_length

def prepare_text_inputs(tokenizer, prompt: str):
    """
    Returns:
      input_ids:      np.int32, shape (1, 77)
      attention_mask: np.int32, shape (1, 77)
    """
    enc = tokenizer(
        prompt,
        padding="max_length",
        truncation=True,
        max_length=MAX_TOKENS,
        return_tensors=None,  # get Python lists, convert manually
    )
    input_ids = np.asarray(enc["input_ids"], dtype=np.int32)           # (1, 77)
    attention_mask = np.asarray(enc["attention_mask"], dtype=np.int32) # (1, 77)
    return input_ids, attention_mask

def setup_parameters(prompt, un_prompt, seed, step, text_guidance):
    """
    Coerce and clamp parameters to valid types/ranges.
    """
    global user_prompt, uncond_prompt, user_seed, user_step, user_text_guidance

    user_prompt = str(prompt or "")
    uncond_prompt = str(un_prompt or "")

    try:
        user_seed = np.int64(seed) if seed is not None else np.int64(42)
    except Exception:
        user_seed = np.int64(42)

    try:
        user_step = int(step) if step is not None else 20
    except Exception:
        user_step = 20

    try:
        user_text_guidance = float(text_guidance) if text_guidance is not None else 7.5
    except Exception:
        user_text_guidance = 7.5

    if user_seed == -1:
        user_seed = np.random.randint(low=0, high=9999999999, size=None, dtype=np.int64)

    # Clamp sensible ranges
    user_step = max(1, min(150, user_step))
    if not (5.0 <= user_text_guidance <= 15.0):
        raise ValueError("user_text_guidance should be a float in [5.0, 15.0]")

def run_scheduler(noise_pred_uncond, noise_pred_text, latent_in, timestep):
    # Convert all inputs from NHWC to NCHW
    noise_pred_uncond = np.transpose(noise_pred_uncond, (0, 3, 1, 2)).copy()
    noise_pred_text = np.transpose(noise_pred_text, (0, 3, 1, 2)).copy()
    latent_in = np.transpose(latent_in, (0, 3, 1, 2)).copy()

    # Convert all inputs to torch tensors
    noise_pred_uncond = torch.from_numpy(noise_pred_uncond)
    noise_pred_text = torch.from_numpy(noise_pred_text)
    latent_in = torch.from_numpy(latent_in)

    # CFG merge
    noise_pred = noise_pred_uncond + user_text_guidance * (noise_pred_text - noise_pred_uncond)

    # Scheduler step
    latent_out = scheduler.step(noise_pred, timestep, latent_in).prev_sample.numpy()

    # Convert latent_out from NCHW to NHWC
    latent_out = np.transpose(latent_out, (0, 2, 3, 1)).copy()

    return latent_out

def get_timestep(step):
    return np.int32(scheduler.timesteps.numpy()[step])

####################################################################
# QNN Model wrappers

class TextEncoder(QNNContext):
    def Inference(self, inputs):
        """
        inputs: list/tuple of numpy arrays in model input order
                e.g., [input_ids_int32_(1,77), attention_mask_int32_(1,77)]
                or [input_ids_int32_(1,77)] if your compiled model is single-input.
        """
        # Normalize to a flat list (avoid double-wrapping like [[...,...]])
        if isinstance(inputs, (list, tuple)):
            input_datas = list(inputs)
        else:
            input_datas = [inputs]

        # Helpful debug logs
        try:
            print("TextEncoder: in shapes/dtypes:",
                  [(getattr(x, 'shape', None), getattr(x, 'dtype', None)) for x in input_datas])
        except Exception:
            pass

        outs = super().Inference(input_datas)
        print("TextEncoder: num outs:", 0 if outs is None else len(outs))

        if not outs:
            raise RuntimeError(
                "QNN Inference returned no outputs — check input count/shape/dtype and model I/O signature"
            )

        output_data = outs[0]

        # Expected output: (1, 77, 768)
        expected = (1, 77, 768)
        if hasattr(output_data, "size") and output_data.size == (1 * 77 * 768) and getattr(output_data, "shape", None) != expected:
            output_data = output_data.reshape(expected)

        return output_data

class Unet(QNNContext):
    def Inference(self, input_data_1, input_data_2, input_data_3):
        # Flatten to 1D for runtime
        input_data_1 = input_data_1.reshape(input_data_1.size)
        input_data_3 = input_data_3.reshape(input_data_3.size)

        input_datas=[input_data_1, input_data_2, input_data_3]
        output_data = super().Inference(input_datas)[0]

        output_data = output_data.reshape(1, 64, 64, 4)
        return output_data

class VaeDecoder(QNNContext):
    def Inference(self, input_data):
        input_data = input_data.reshape(input_data.size)
        input_datas=[input_data]

        output_data = super().Inference(input_datas)[0]
        return output_data

####################################################################
# Init / Destroy

def model_initialize():
    global scheduler, tokenizer, text_encoder, unet, vae_decoder

    result = True

    SetQNNConfig()
    model_download()

    # model names for QNN contexts
    model_text_encoder  = "text_encoder"
    model_unet          = "model_unet"
    model_vae_decoder   = "vae_decoder"

    # Initializing the Tokenizer (prefer local cache)
    try:
        if os.path.exists(tokenizer_dir) and not os.path.exists(os.path.join(tokenizer_dir, ".locks")):
            tokenizer = CLIPTokenizer.from_pretrained(tokenizer_dir, local_files_only=True)
        elif os.path.exists(tokenizer_dir):
            tokenizer = CLIPTokenizer.from_pretrained(TOKENIZER_MODEL_NAME, cache_dir=tokenizer_dir, local_files_only=True)
        else:
            tokenizer = CLIPTokenizer.from_pretrained(TOKENIZER_MODEL_NAME, cache_dir=tokenizer_dir)
        print(">>> Loaded tokenizer:", TOKENIZER_MODEL_NAME)
    except Exception as e:
        fail = "\nFailed to download tokenizer model. Please prepare the tokenizer data according to the guide below:\n" + TOKENIZER_HELP_URL + "\n"
        print(fail)
        raise

    # QNN model instances
    text_encoder = TextEncoder(model_text_encoder, text_encoder_model_path)
    unet = Unet(model_unet, unet_model_path)
    vae_decoder = VaeDecoder(model_vae_decoder, vae_decoder_model_path)

    # Scheduler
    scheduler = DPMSolverMultistepScheduler(num_train_timesteps=1000, beta_start=0.00085, beta_end=0.012, beta_schedule="scaled_linear")

    # Optional self-test for text encoder
    try:
        ids, mask = prepare_text_inputs(tokenizer, "a photo of a cat on a sofa")
        emb = text_encoder.Inference([ids, mask])  # falls back to single input later if needed
        print(">>> Encoder self-test out:", getattr(emb, "shape", None), getattr(emb, "dtype", None))
    except Exception as e:
        print(">>> Encoder self-test failed:", e)

    return result

def model_destroy():
    global text_encoder
    global unet
    global vae_decoder

    del(text_encoder)
    del(unet)
    del(vae_decoder)

def SetQNNConfig():
    QNNConfig.Config(qnn_dir, Runtime.HTP, LogLevel.ERROR, ProfilingLevel.BASIC)

####################################################################
# Execute the Stable Diffusion pipeline

def model_execute(callback, image_dir, show_image = True):
    PerfProfile.SetPerfProfileGlobal(PerfProfile.BURST)

    scheduler.set_timesteps(user_step)  # Setting user-provided timesteps

    # Tokenize (ids + mask as int32)
    cond_ids, cond_mask = prepare_text_inputs(tokenizer, user_prompt)
    uncond_ids, uncond_mask = prepare_text_inputs(tokenizer, uncond_prompt)

    print("Caller cond:", cond_ids.shape, cond_ids.dtype, cond_mask.shape, cond_mask.dtype)
    print("Caller uncond:", uncond_ids.shape, uncond_ids.dtype, uncond_mask.shape, uncond_mask.dtype)

    # Encode text (try two inputs; fallback to single input if compiled that way)
    try:
        uncond_text_embedding = text_encoder.Inference([uncond_ids, uncond_mask])
        user_text_embedding   = text_encoder.Inference([cond_ids, cond_mask])
    except RuntimeError as e:
        print("TextEncoder two-input failed (input_ids + attention_mask). Falling back to single-input. Err:", e)
        uncond_text_embedding = text_encoder.Inference([uncond_ids])
        user_text_embedding   = text_encoder.Inference([cond_ids])

    # Initialize latents
    random_init_latent = torch.randn((1, 4, 64, 64), generator=torch.manual_seed(int(user_seed))).numpy()
    latent_in = random_init_latent.transpose(0, 2, 3, 1)

    # Denoising loop
    for step in range(user_step):
        print(f'Step {step} Running...')
        time_step = get_timestep(step)

        unconditional_noise_pred = unet.Inference(latent_in, time_step, uncond_text_embedding)
        conditional_noise_pred = unet.Inference(latent_in, time_step, user_text_embedding)

        latent_in = run_scheduler(unconditional_noise_pred, conditional_noise_pred, latent_in, time_step)

        callback(step)

    # Decode with VAE
    now = datetime.datetime.now()
    output_image = vae_decoder.Inference(latent_in)
    formatted_time = now.strftime("%Y_%m_%d_%H_%M_%S")

    if len(output_image) == 0:
        callback(None)
        PerfProfile.RelPerfProfileGlobal()
        return None
    else:
        image_size = 512

        if not os.path.exists(image_dir):
            os.makedirs(image_dir, exist_ok=True)
        out_path = os.path.join(image_dir, f"{formatted_time}_{str(user_seed)}_{str(image_size)}.jpg")

        output_image = np.clip(output_image * 255.0, 0.0, 255.0).astype(np.uint8)
        output_image = output_image.reshape(image_size, image_size, -1)
        output_image = Image.fromarray(output_image, mode="RGB")
        output_image.save(out_path)

        if show_image:
            output_image.show()

        callback(out_path)

        PerfProfile.RelPerfProfileGlobal()
        return out_path

####################################################################
# Progress callback

def modelExecuteCallback(result):
    if ((None == result) or isinstance(result, str)):
        if (None == result):
            result = "None"
        else:
            print("Image saved to '" + result + "'")
    else:
        result = (result + 1) * 100
        result = int(result / user_step)
        result = str(result)

####################################################################
# Model download: currently not in use

def model_download():
    desc = f"Downloading {MODEL_NAME} model... "
    fail = f"\nFailed to download {MODEL_NAME} model. Please prepare the models according to the steps in below link:\n{MODEL_HELP_URL}"

    install.download_qai_hubmodel(MODEL_ID_VAE, vae_decoder_model_path, desc=desc, fail=fail, hub_id=HUB_ID_H)
    install.download_qai_hubmodel(MODEL_ID_UNET, unet_model_path, desc=desc, fail=fail, hub_id=HUB_ID_H)
    install.download_qai_hubmodel(MODEL_ID_TEXT, text_encoder_model_path, desc=desc, fail=fail, hub_id=HUB_ID_H)

    desc2 = ("Please download Stable-Diffusion-v1.5 model from "
             "https://aihub.qualcomm.com/compute/models/stable_diffusion_v1_5 "
             "and save them to path 'samples\\python\\stable_diffusion_v1_5\\models'.\n")
    if not os.path.exists(text_encoder_model_path) or not os.path.exists(unet_model_path) or not os.path.exists(vae_decoder_model_path):
        print(desc2)
        raise FileNotFoundError("QNN model files are missing")

####################################################################
# Flask route

@app.route('/generate', methods=['POST'])
def generate_image():
    data = request.get_json(silent=True) or {}

    prompt = data.get("prompt", "")
    seed = data.get("seed", 42)
    steps = data.get("steps", 20)
    # let client pass guidance if you want: float in [5.0, 15.0]
    guidance = data.get("guidance", 7.5)

    # Standard negative prompt
    uncond = ("lowres, text, error, cropped, worst quality, low quality, normal quality, "
              "jpeg artifacts, signature, watermark")

    setup_parameters(prompt, uncond, seed, steps, guidance)

    # Call pipeline
    image_dir = os.path.join(execution_ws, "images")
    generated_image_path = model_execute(modelExecuteCallback, image_dir)

    if isinstance(generated_image_path, str) and os.path.exists(generated_image_path):
        return send_file(generated_image_path, mimetype='image/jpeg')
    else:
        return jsonify({"error": "Image generation failed"}), 500

####################################################################

if __name__ == '__main__':
    model_initialize()
    app.run(host='0.0.0.0', port=8082)
    model_destroy()
