# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------
import argparse
import numpy as np
import torch
import soundfile as sf
from melo.api import TTS
import os
import time 
import nltk
try:
    nltk.data.find('taggers/averaged_perceptron_tagger_eng')
except LookupError:
    nltk.download('averaged_perceptron_tagger_eng')


torch.manual_seed(0)
np.random.seed(0)
from OnnxRunnerHelper import ONNXRunner
MAX_SEQ_LEN = 512
MAX_NUM_INPUT_IDS = 50
NUM_BLOCKS =4
MAX_DEC_SEQ_LEN = 40
DEC_SEQ_OVERLAP = 12
UPSAMPLE_FACTOR = 512
BUF_DEBUG = 1
model = None
language = None

import logging
from flask import Flask, request, send_file, jsonify

logger = logging.getLogger("meloTTS")
app = Flask(__name__)

if not logger.handlers:
    handler = logging.StreamHandler()
    formatter = logging.Formatter('%(asctime)s - %(module)s:%(lineno)d - %(levelname)s - %(message)s')
    handler.setFormatter(formatter)
    logger.addHandler(handler)

logger.setLevel(logging.INFO)  



def generate_path(duration, mask):
    b, _, t_y, t_x = mask.shape
    cum_duration = np.cumsum(duration, axis=-1)
    time_indices = np.arange(t_y)[None, None, None, :]
    cum_duration_broadcast = cum_duration[:, :, :, None]
    path = (time_indices < cum_duration_broadcast).astype(np.float32)
    path = np.diff(path, axis=2, prepend=0)
    path = path.transpose(0,1,3,2)
    return path * mask

class OnnxTTS:

    def __init__(self, encoder_model_path, flow_model_path, decoder_model_path,
                 charsiu_encoder_path=None,charsiu_decoder_path=None,bert_path=None,
                 language="ENGLISH",ep='cpu',backend_path=None,debug=False,
                 gen_ctx=None):
        self.ep =ep
        self.backend_path =backend_path
        self.debug=debug

        self.generate_context =gen_ctx
        # self.enable_g2p =enable_g2p
        self.ort_runer = ONNXRunner()

        self.encoder = self.ort_runer.create_ort_session(encoder_model_path,"Encoder",self.ep,self.backend_path,self.debug)
        self.flow = self.ort_runer.create_ort_session(flow_model_path,"Flow",self.ep,self.backend_path,self.debug)
        self.decoder = self.ort_runer.create_ort_session(decoder_model_path,"Decoder",self.ep,self.backend_path,self.debug)
        
        self.encoder_input_names,self.encoder_output_names =self.ort_runer.get_input_and_output_names(self.encoder)
        self.decoder_input_names,self.decoder_output_names =self.ort_runer.get_input_and_output_names(self.decoder)
        self.flow_input_names,self.flow_output_names =self.ort_runer.get_input_and_output_names(self.flow)

        self.enable_bert =False
        if language == "ENGLISH":
            self.TTS_language = "EN_NEWEST"
            self.enable_bert =True
        elif language == "SPANISH":
            self.TTS_language = "ES"
        # elif language == "CHINESE":
        #     self.TTS_language = "ZH"

        self.tts = TTS(language=self.TTS_language, device="cpu")
        self.language = language
        self.max_seq_len = MAX_SEQ_LEN
        self.get_text_for_tts_infer =None
        if self.enable_bert:
            from melo.text import cleaned_text_to_sequence
            from melo.text.cleaner import clean_text
            from melo import commons
            from transformers import AutoTokenizer
            self.text_normalize = None
            if language =="ENGLISH":
                from melo.text.english import text_normalize
                self.text_normalize =text_normalize
            # elif language == "CHINESE": 
            #     from melo.text.chinese import text_normalize
            #     self.text_normalize =text_normalize
            self.commons =commons
            self.model_id = 'bert-base-uncased'
            self.sent_tokenizer = AutoTokenizer.from_pretrained(self.model_id)
            self.clean_text = clean_text
            self.cleaned_text_to_sequence = cleaned_text_to_sequence
            self.bert = self.ort_runer.create_ort_session(bert_path,"BERT",self.ep,self.backend_path,self.debug)
            self.bart_input_names,self.bart_output_names = self.ort_runer.get_input_and_output_names(self.bert)
        else:
            from melo.utils import get_text_for_tts_infer
            self.get_text_for_tts_infer = get_text_for_tts_infer

            
    
    def get_bert_feature(self,text, word2ph):     
        logger.debug(f"text : {text}")
        inputs = self.sent_tokenizer(text, padding='max_length', max_length=200, return_tensors="pt")
        inputs = {k: v.numpy() for k, v in inputs.items()}
        bert_input = self.ort_runer.get_onnx_model_input(self.bert, list(inputs.keys()),list(inputs.values()))
        res, self.bert_exe_time = self.ort_runer.execute(self.bert,bert_input)
        res = torch.tensor(res[0].squeeze() )
        logger.debug("res : ",res.shape)
        logger.debug(len(word2ph))
        logger.debug("word2ph : ",word2ph)
        word2phone = word2ph
        phone_level_feature = []
        for i in range(len(word2phone)):
            repeat_feature = res[i].repeat(word2phone[i], 1)
            phone_level_feature.append(repeat_feature)
        phone_level_feature = torch.cat(phone_level_feature, dim=0)
        return phone_level_feature.T


    # def clean_text_modify(self,text):
    #     norm_text = self.text_normalize(text)
    #     phones, tones, word2ph = self.g2p_modify(norm_text)
    #     return norm_text, phones, tones, word2ph

    def get_text_for_tts_infer_modify(self,text, language_str, hps, device, symbol_to_id=None):
        # if self.text_normalize is None:
        #     norm_text, phone, tone, word2ph = self.clean_text(text, language_str)
        # else:
        #     norm_text, phone, tone, word2ph = self.clean_text_modify(text)
        norm_text, phone, tone, word2ph = self.clean_text(text, language_str)
        phone, tone, language = self.cleaned_text_to_sequence(phone, tone, language_str, symbol_to_id)

        if hps.data.add_blank:
            phone = self.commons.intersperse(phone, 0)
            tone = self.commons.intersperse(tone, 0)
            language = self.commons.intersperse(language, 0)
            for i in range(len(word2ph)):
                word2ph[i] = word2ph[i] * 2
            word2ph[0] += 1

        logger.debug("hps.data : ",hps.data)
        if getattr(hps.data, "disable_bert", False):
            bert = torch.zeros(1024, len(phone))
            ja_bert = torch.zeros(768, len(phone))
        else:
            bert = self.get_bert_feature(norm_text, word2ph)
            del word2ph
            assert bert.shape[-1] == len(phone), phone

            if language_str == "ZH":
                bert = bert
                ja_bert = torch.zeros(768, len(phone))
            elif language_str in ["JP", "EN", "ZH_MIX_EN", 'KR', 'SP', 'ES', 'FR', 'DE', 'RU']:
                ja_bert = bert
                bert = torch.zeros(1024, len(phone))
            else:
                raise NotImplementedError()

        assert bert.shape[-1] == len(
            phone
        ), f"Bert seq len {bert.shape[-1]} != {len(phone)}"

        phone = torch.LongTensor(phone)
        tone = torch.LongTensor(tone)
        language = torch.LongTensor(language)
        return bert, ja_bert, phone, tone, language
    
    def preprocess_text(self, text):
        if self.get_text_for_tts_infer is None:
            logger.debug("self.tts.language : ",self.tts.language)
            bert, ja_bert, phones, tones, lang_ids = self.get_text_for_tts_infer_modify(text, self.tts.language, self.tts.hps, "cpu", self.tts.symbol_to_id)
        else : 
            bert, ja_bert, phones, tones, lang_ids = self.get_text_for_tts_infer(text, self.tts.language, self.tts.hps, "cpu", self.tts.symbol_to_id)      
        
        logger.debug(f"phones : {phones}, len = {len(phones)}")
        logger.debug(f"tones : {tones}, len = {len(tones)}")
        logger.debug(f"language : {lang_ids}, len = {len(lang_ids)}")

        phone_len = phones.size(0)
        phones = torch.nn.functional.pad(phones, (0, self.max_seq_len - phones.size(0)))[:self.max_seq_len]
        tones = torch.nn.functional.pad(tones, (0, self.max_seq_len - tones.size(0)))[:self.max_seq_len]
        lang_ids = torch.nn.functional.pad(lang_ids, (0, self.max_seq_len - lang_ids.size(0)))[:self.max_seq_len]
        bert = torch.nn.functional.pad(bert, (0, self.max_seq_len - bert.size(1), 0, 0))[:, :self.max_seq_len]
        ja_bert = torch.nn.functional.pad(ja_bert, (0, self.max_seq_len - ja_bert.size(1), 0, 0))[:, :self.max_seq_len]
        
        return phones, tones, lang_ids, bert, ja_bert, phone_len
        
    def tts_to_file(self, text, speaker_id, output_path, noise_scale=0.667, length_scale=1.0, noise_scale_w=0.8, sdp_ratio=0.2):
        pipe_start_time = time.time()
        pre_start_time = time.time()
        phones, tones, lang_ids, bert, ja_bert, phone_len = self.preprocess_text(text)
        pre_exe_time = ( time.time() - pre_start_time )*1000
        
        inputs = {
            'x': phones.unsqueeze(0).numpy(),
            'x_lengths': np.array([phone_len], dtype=np.int64),
            'sid': np.array([speaker_id], dtype=np.int64),
            'tone': tones.unsqueeze(0).numpy(),
            'language': lang_ids.unsqueeze(0).numpy(),
            'bert': bert.unsqueeze(0).numpy(),
            'ja_bert': ja_bert.unsqueeze(0).numpy(),
            'sdp_ratio': np.array([sdp_ratio], dtype=np.float32), 
            'length_scale': np.array([length_scale], dtype=np.float32),
            'noise_scale_w': np.array([noise_scale_w], dtype=np.float32),
        }


        logger.info("Executing Encoder : ...")
        encoder_input = self.ort_runer.get_onnx_model_input(self.encoder,list(inputs.keys()),list(inputs.values()))
        encoder_output,enc_exe_time = self.ort_runer.execute(self.encoder,encoder_input,['y_lengths', 'x_mask', 'm_p', 'logs_p', 'g', 'w_ceil'])
        y_lengths, x_mask, m_p, logs_p, g, w_ceil = encoder_output
        y_mask = np.expand_dims(np.arange(MAX_SEQ_LEN*3) < y_lengths[:, None], axis=1).astype(np.float32)
        attn_mask = np.expand_dims(x_mask, axis=2) * np.expand_dims(y_mask, axis=-1)
        attn = generate_path(w_ceil, attn_mask)
        attn_squeezed = attn.squeeze(1)
        
        flow_inputs = {
                "m_p": m_p.astype(np.float32),
                "logs_p": logs_p.astype(np.float32),
                "y_mask": y_mask,
                "g": g,
                "attn_squeezed": attn_squeezed.astype(np.float32),
                'noise_scale': np.array([noise_scale], dtype=np.float32),
            }

        logger.info("Executing Flow : ...")
        flow_input = self.ort_runer.get_onnx_model_input(self.flow,list(flow_inputs.keys()),list(flow_inputs.values()))
        flow_output,flow_exe_time = self.ort_runer.execute(self.flow,flow_input)
        z = flow_output[0]

        decoder_inputs = {
            "z" : z,
            "g": g
        }
        i = 0
   
        dec_seq_len = MAX_DEC_SEQ_LEN
        dec_seq_overlap = DEC_SEQ_OVERLAP

        z_buf = np.zeros([z.shape[0], z.shape[1], MAX_DEC_SEQ_LEN + 2 * DEC_SEQ_OVERLAP]).astype(np.float32)
        z_buf[:,:,:(dec_seq_len+dec_seq_overlap)] = z[:,:,:(dec_seq_len+dec_seq_overlap)]
        #print(z_buf.shape)
        decoder_inputs = {
            "z" : z_buf,
            "g": g
        }

        logger.info("Executing Decoder : ...")
        decoder_input = self.ort_runer.get_onnx_model_input(self.decoder,list(decoder_inputs.keys()),list(decoder_inputs.values()))
        decoder_output , first_dec_exe_time = self.ort_runer.execute(self.decoder,decoder_input)
        audio_chunk = decoder_output[0]
        first_decoder_time =first_dec_exe_time+enc_exe_time+flow_exe_time
            
        upsample_factor = 512
        audio = audio_chunk.squeeze()[:dec_seq_len*upsample_factor]
        total_dec_seq_len = dec_seq_len
        total_dec_exection_time =first_dec_exe_time
        total_token_count =0
        while (total_dec_seq_len < y_lengths):
            total_token_count +=1
            z_buf = z[:,:,total_dec_seq_len-DEC_SEQ_OVERLAP:total_dec_seq_len+MAX_DEC_SEQ_LEN+DEC_SEQ_OVERLAP]
            #print(z_buf.shape)
            decoder_inputs = {
            "z" : z_buf,
            "g": g
        }

            decoder_input = self.ort_runer.get_onnx_model_input(self.decoder,list(decoder_inputs.keys()),list(decoder_inputs.values()))
            decoder_output , first_dec_exe_time = self.ort_runer.execute(self.decoder,decoder_input)
            audio_chunk = decoder_output[0]
            total_dec_exection_time +=first_dec_exe_time
            #print(audio_chunk.shape)
            audio_chunk = audio_chunk.squeeze()[DEC_SEQ_OVERLAP * UPSAMPLE_FACTOR:(MAX_DEC_SEQ_LEN+DEC_SEQ_OVERLAP) * UPSAMPLE_FACTOR]
            audio = np.concatenate([audio, audio_chunk])
            #print(audio.shape)
            total_dec_seq_len += dec_seq_len

        length = int(y_lengths[0])*512
        
        
        audio = audio.squeeze()[:length]
        pipe_end_time = time.time()
        sf.write(output_path, audio.squeeze(), samplerate=self.tts.hps.data.sampling_rate)
        print(f"\n**********Summary {self.ep.upper()}***********\n")
        print(f"Bert Execution Time : {self.bert_exe_time:.4f} ms")
        print(f"Encoder Execution Time : {enc_exe_time:.4f}ms")
        print(f"Flow Execution Time : {flow_exe_time:.4f}ms")
        print(f"First Decoder Execution Time : {(first_decoder_time+enc_exe_time):.4f}ms")
        print(f"Total Decoder Execution Time : {total_dec_exection_time:.4f}ms")
        print(f"Total Model Execution Time : {(self.bert_exe_time+enc_exe_time+flow_exe_time+total_dec_exection_time):.4f} ms")
        # print(f"Total Pipeline Execution Time : {(pipe_end_time - pipe_start_time):.4f} sec")
        print(f"Total Token : {total_token_count} ")
        decoder_speed_tokens_per_sec = total_token_count / (total_dec_exection_time / 1000)
        print(f"Decoder speed: {decoder_speed_tokens_per_sec:.4f} token/sec")
        # print(f"Preprocessing Execution Time : {pre_exe_time:.4f} sec")

        print(f"\n*****************************************************\n")

        
@app.route('/generate', methods=['POST'])
def inference():
    global language, model
    data = request.get_json()
    text = data.get("text", "")
    if not text:
        return jsonify({"error": "No text provided"}), 400


    result = os.path.join("result",language)
    os.makedirs(result,exist_ok=True)
    if not isinstance(text,list):
        text =[text]
    
    for idx, input_text,in enumerate(text):
        logger.info(f"text : {input_text}")
        output_path = os.path.join(result, f"test-onnx-output_{language}.wav")
        model.tts_to_file(input_text, speaker_id=0, output_path=output_path)
        logger.info(f"Audio generated and saved to {output_path}")
        
        # Send the .wav file
        return send_file(
            output_path,
            mimetype="audio/wav",
            as_attachment=False
        )
 



def main(args):
    global language, model
    language = args.language.upper()
    ep = args.ep
    backend_path = args.backend_path
    debug = args.debug
    working_dir = args.working_dir
    
    if language =="ENGLISH":
        text = "This is an example of text to speech using Melo for English. How does it sound?"
        if ep=="cpu":
            # for testing exported model 
            charsiu_decoder_path = os.path.join(working_dir,'models', "charsiu_decoder.onnx")
            charsiu_encoder_path = os.path.join(working_dir,'models',"charsiu_encoder.onnx")
            decoder_model_path = os.path.join(working_dir,'models',"decoder_ENGLISH.onnx")
            encoder_model_path= os.path.join(working_dir,'models',"encoder_ENGLISH.onnx")
            flow_model_path =os.path.join(working_dir,'models',"flow_ENGLISH.onnx")
            bert_model_path =os.path.join(working_dir,'models',"bert_ENGLISH.onnx")
        else:
            charsiu_decoder_path = os.path.join(working_dir,'models',"charsiu_decoder_net_qnn_ctx.onnx")
            charsiu_encoder_path = os.path.join(working_dir,'models',"charsiu_encoder_net_qnn_ctx.onnx")
            decoder_model_path = os.path.join(working_dir,'models',"decoder_net_qnn_ctx.onnx")
            encoder_model_path= os.path.join(working_dir,'models',"encoder_net_qnn_ctx.onnx")
            flow_model_path =os.path.join(working_dir,'models',"flow_net_qnn_ctx.onnx")
            bert_model_path =os.path.join(working_dir,'models',"bert_net_qnn_ctx.onnx")
    elif language =="SPANISH":
        text = "Este es un ejemplo de conversión de texto a voz con Melo para español. ¿Cómo suena?" 
        if ep=="cpu":
            charsiu_decoder_path = os.path.join(working_dir,'models', "charsiu_decoder.onnx")
            charsiu_encoder_path = os.path.join(working_dir,'models',"charsiu_encoder.onnx")
            decoder_model_path = os.path.join(working_dir,'models',"decoder_SPANISH.onnx")
            encoder_model_path= os.path.join(working_dir,'models',"encoder_SPANISH.onnx")
            flow_model_path =os.path.join(working_dir,'models',"flow_SPANISH.onnx")
            bert_model_path =None
        else:

            charsiu_decoder_path = os.path.join(working_dir,'models', "charsiu_decoder.onnx")
            charsiu_encoder_path = os.path.join(working_dir,'models',"charsiu_encoder.onnx")
            bert_model_path =None
            decoder_model_path = os.path.join(working_dir,'models',"decoder_net_qnn_ctx.onnx")
            encoder_model_path= os.path.join(working_dir,'models',"encoder_net_qnn_ctx.onnx")
            flow_model_path =os.path.join(working_dir,'models',"flow_net_qnn_ctx.onnx")
           

    model_list =[charsiu_decoder_path,charsiu_encoder_path,decoder_model_path,encoder_model_path,flow_model_path,bert_model_path]
    for model_file in model_list:
        if model_file is not None:
            if not os.path.exists:
                raise FileNotFoundError(f"The specified file was not found at : {model_file}")

    
    

    model = OnnxTTS(encoder_model_path, flow_model_path, decoder_model_path,charsiu_encoder_path, charsiu_decoder_path,bert_model_path,
                    language, # language_map[language],
                    ep=ep, backend_path=backend_path,debug=debug)
    
    logger.info(f"Language : {language}")
    logger.info(f"ep : {ep}")
   
    logger.info(f"backend_path : {backend_path}")
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run MeloTTS  Text-to-Speech")
    parser.add_argument("-ep", choices=["cpu", "npu"], default="npu", help="Execution provider")
    parser.add_argument("-t","--text",  default=None, help="text to generated speech")
    parser.add_argument("-l","--language",  default="english", help="speech language")
    # parser.add_argument('-g2p',"--enable_g2p", action="store_true", help="Enable G2P processing")
    parser.add_argument("-b","--backend_path", default=None, help="Path to backend, if applicable")
    parser.add_argument("-d","--debug", action="store_true", help="Enable debug mode")
    parser.add_argument("-wd","--working_dir", default=os.getcwd(),help="Working directory containing models and output")

    args = parser.parse_args()
    main(args)
    app.run(host='0.0.0.0', port=8083)
