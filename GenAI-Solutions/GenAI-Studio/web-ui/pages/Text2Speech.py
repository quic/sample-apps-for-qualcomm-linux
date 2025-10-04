# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------

import streamlit as st
import requests
import io

# Set page config
st.set_page_config(page_title="GenAI Studio")

# Page-specific key prefix
PAGE_KEY = "tts"

# Sidebar layout
with st.sidebar:
    st.title('GenAI Studio')
    st.subheader('Text to Audio')
    st.markdown('More info [doc](https://docs.qualcomm.com/bundle/publicresource/topics/80-70018-115/qualcomm-linux-docs-home.html?vproduct=1601111740013072&version=1.4)!')

# Server URL for TTS
server_url = "http://0.0.0.0:8083/generate"

# Initialize chat history
if f"{PAGE_KEY}_messages" not in st.session_state:
    st.session_state[f"{PAGE_KEY}_messages"] = []

# Display history
st.subheader("Text-to-Speech")
for message in st.session_state[f"{PAGE_KEY}_messages"]:
    with st.chat_message("user"):
        st.write(message["text"])
    with st.chat_message("assistant"):
        st.audio(message["audio"], format="audio/mp3")
    st.markdown("---")

# Text input
text_input = st.text_area("Enter text to convert to speech")

# Button to trigger TTS
if st.button("🔊 Generate Audio"):
    if text_input.strip():
        with st.spinner("Generating audio..."):
            try:
                # Send POST request with text
                response = requests.post(server_url, json={"text": text_input})

                if response.status_code == 200:
                    # Convert response content to BytesIO for playback
                    audio_bytes = io.BytesIO(response.content)

                    # Save to history
                    st.session_state[f"{PAGE_KEY}_messages"].append({
                        "text": text_input,
                        "audio": audio_bytes
                    })

                    st.success("Audio generated successfully!")
                    st.audio(audio_bytes, format="audio/mp3")  # or "audio/wav" depending on backend
                else:
                    st.error(f"Error: {response.status_code} - {response.text}")
            except Exception as e:
                st.error(f"Request failed: {e}")
    else:
        st.warning("Please enter some text.")
