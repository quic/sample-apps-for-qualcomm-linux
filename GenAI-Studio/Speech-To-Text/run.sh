# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------

#! /bin/bash
. ~/miniconda3/bin/activate
conda activate py312
export LD_LIBRARY_PATH=/app/Speech-To-Text/ai-engine-direct-helper/samples/python/qai_libs/
export ADSP_LIBRARY_PATH=/app/Speech-To-Text/ai-engine-direct-helper/samples/python/qai_libs/
python3 whisper_base_en/whisper_base_en.py