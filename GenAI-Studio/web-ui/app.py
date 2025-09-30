# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------

import streamlit as st

st.set_page_config(page_title="Main Page", layout="wide")
st.title("GenAI Studio")

col1, col2, col3 = st.columns(3)
with col1:
    st.image("static/assets/ASR.png", use_container_width=True)
    if st.button("\U0001F399 Automatic speech recognition (ASR)"):
        st.switch_page("pages/ASR.py")
with col2:
    st.image("static/assets/Text2Image.png", use_container_width=True)
    if st.button("\U0001F3A8 Text2Image"):
        st.switch_page("pages/Text2Image.py")
with col3:
    st.image("static/assets/Text2Text.png",  use_container_width=True)
    if st.button("\U0001F4DD Text2Text"):
        st.switch_page("pages/Text2Text.py")