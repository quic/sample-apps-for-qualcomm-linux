# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------

import onnxruntime as ort
import numpy as np
import time
import logging

logger = logging.getLogger("OnnxRuntimeHelper")

if not logger.handlers:
    handler = logging.StreamHandler()
    formatter = logging.Formatter('%(asctime)s - %(module)s:%(lineno)d - %(levelname)s - %(message)s')
    handler.setFormatter(formatter)
    logger.addHandler(handler)

class ONNXRunner:
    def __init__(self):
        pass

    def create_ort_session(self, model_path, model_name, ep, backend_path, debug_log):
        session_opts = ort.SessionOptions()
        if debug_log:
            session_opts.log_severity_level = 0
        if ep == 'cpu':
            start_time = time.time()
            session = ort.InferenceSession(model_path, sess_options=session_opts)
            end_time = time.time()
        elif ep == 'npu':
            backend_path = "/usr/lib/libQnnHtp.so" if backend_path is None else backend_path
            if hasattr(self, 'generate_context') and self.generate_context:
                session_opts.add_session_config_entry("ep.context_enable", "1")
            PROVIDER_OPTIONS = [{
                "backend_path": backend_path,
                "htp_performance_mode": "burst",
            }]
            EXECUTION_PROVIDER = ["QNNExecutionProvider"]
            start_time = time.time()
            session = ort.InferenceSession(
                model_path,
                sess_options=session_opts,
                providers=EXECUTION_PROVIDER,
                provider_options=PROVIDER_OPTIONS
            )
            end_time = time.time()
        else:
            logger.error(f"Unsupported execution provider: {ep}")
            return None

        session_creation_time = (end_time - start_time) * 1000
        if logger.isEnabledFor(logging.DEBUG):
            self.display_model_info(session, framework=ep)
        logger.info(f"Session Creation Time {model_name} : {session_creation_time:.4f} ms")
        logger.debug("************************************************************************************")
        return session
    
    def get_input_and_output_names(self, session):
        input_names = [node.name for node in session.get_inputs()]
        output_names = [node.name for   node in session.get_outputs()]
        return input_names, output_names
    
    def get_random_input_data(self, session):
        model_input_list = []
        input_data_list = [[]]
        for inp_lists in input_data_list:
            input_data = {}
            for i, node in enumerate(session.get_inputs()):
                input_shape = node.shape
                np_dtype = self.get_input_data(node.type)
                input_name = node.name
                img = np.random.uniform(low=0.01, high=1.0, size=(input_shape))
                if np_dtype == np.int8:
                    img *= 255
                elif np_dtype == np.int16:
                    img *= 65535
                img = img.astype(np_dtype)
                input_data[input_name] = img
            model_input_list.append(input_data)
        return model_input_list[0] if len(model_input_list) == 1 else model_input_list

    def get_onnx_model_input(self, session, inp_name_list, data_list):
        result = {}
        for sess in session.get_inputs():
            new_name = ''
            input_name = sess.name
            un_quant_name = input_name.replace("_dq", '') if "_dq" in input_name else input_name
            if input_name in inp_name_list:
                new_name = input_name
            elif un_quant_name in inp_name_list:
                new_name = un_quant_name
            else:
                logger.error(f"Session name {input_name} not in passed name list: {inp_name_list}")
                return None
            dtype = self.get_input_data(str(sess.type))
            result[input_name] = data_list[inp_name_list.index(new_name)].astype(dtype)
            logger.debug(f"name : {input_name} {result[input_name].shape}, {type(result[input_name])} {result[input_name].dtype}")
        return result

    def get_input_data(self, onnx_dtype):
        if onnx_dtype == "tensor(float)":
            return np.float32
        elif onnx_dtype == "tensor(float16)":
            return np.float16
        elif onnx_dtype == "tensor(uint64)":
            return np.uint64
        elif onnx_dtype == "tensor(int64)":
            return np.int64
        elif onnx_dtype == "tensor(uint32)":
            return np.uint32
        elif onnx_dtype == "tensor(int32)":
            return np.int32
        else:
            logger.error(f"Datatype not implemented: {onnx_dtype}")
            return None

    def display_model_info(self, session, framework='CPU'):
        logger.debug("Model Input:")
        for node in session.get_inputs():
            logger.debug(f"name: {node.name}, shape: {node.shape}, dtype: {self.get_input_data(node.type)}")
        logger.debug("Model Output:")
        for node in session.get_outputs():
            logger.debug(f"name: {node.name}, shape: {node.shape}, dtype: {self.get_input_data(node.type)}")

    def execute(self, session, input_data, output_name=None):
        start_time = time.time()
        output = session.run(output_name, input_data)
        end_time = time.time()
        enc_time = (end_time - start_time) * 1000
        return output, enc_time
