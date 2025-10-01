# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------

import streamlit as st
import requests

# Set page config
st.set_page_config(page_title="GenAI Studio")

# Page-specific key prefix
PAGE_KEY = "asr"

# Sidebar layout
with st.sidebar:
    st.title('GenAI Studio')
    st.subheader('Solution')
    st.markdown('More info [doc](https://docs.qualcomm.com/bundle/publicresource/topics/80-70018-115/qualcomm-linux-docs-home.html?vproduct=1601111740013072&version=1.4)!')

    st.title("Generation Settings")
    if st.button("🧹 Clear Chat History"):
        st.session_state[f"{PAGE_KEY}_messages"] = [{
            "role": "assistant",
            "content": "Hey there! 😄"
        }]
        st.rerun()

# Server URL
server_url = 'http://0.0.0.0:8081/generate'

# Initialize page-specific chat history
if f"{PAGE_KEY}_messages" not in st.session_state:
    st.session_state[f"{PAGE_KEY}_messages"] = [{
        "role": "assistant",
        "content": "Hey there! 😄"
    }]

# Display chat history
st.subheader("Chat History")
for message in st.session_state[f"{PAGE_KEY}_messages"]:
    with st.chat_message(message["role"]):
        st.write(f"{message['content']}")
    st.markdown("---")

# File uploader for audio
audio_file = st.file_uploader("Upload a .wav audio file", type=["wav"])

if audio_file is not None:
    st.audio(audio_file, format="audio/wav")

    if st.button("🎙 Generate Text from Audio"):
        with st.spinner("Transcribing audio..."):
            try:
                files = {"audio": (audio_file.name, audio_file, "audio/wav")}
                response = requests.post(server_url, files=files)

                if response.status_code == 200:
                    data = response.json()
                    result_text = data["text"]
                    result_msg = data["message"]

                    # Append to page-specific history
                    st.session_state[f"{PAGE_KEY}_messages"].append({
                        "role": "user",
                        "content": f"Uploaded: {audio_file.name}"
                    })
                    st.session_state[f"{PAGE_KEY}_messages"].append({
                        "role": "assistant",
                        "content": f"{result_text}\n\n{result_msg}"
                    })

                    st.success(result_msg)
                    st.markdown(result_text)
                else:
                    st.error(f"Error: {response.status_code} {response.text}")
            except Exception as e:
                st.error(f"Request failed: {e}")
