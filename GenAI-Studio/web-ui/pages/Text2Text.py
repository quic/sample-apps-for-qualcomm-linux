# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------

import streamlit as st
import requests
import json

# Set page config
st.set_page_config(page_title="GenAI Studio")

# Page-specific key prefix
PAGE_KEY = "text2text"

# Sidebar layout
with st.sidebar:
    st.title('GenAI Studio')
    st.subheader('Solution')
    st.markdown('More info doc!')
    st.markdown('© Qualcomm Technologies, Inc.')
    st.markdown('---')

    if st.button("🧹 Clear Chat History"):
        st.session_state[f"{PAGE_KEY}_messages"] = [{
            "role": "assistant",
            "content": "Hey there! 😄 How can I assist you today?"
        }]
        st.rerun()

# Server URL
server_url = 'http://0.0.0.0:8088/process'

# Initialize page-specific chat history
if f"{PAGE_KEY}_messages" not in st.session_state:
    st.session_state[f"{PAGE_KEY}_messages"] = [{
        "role": "assistant",
        "content": "Hey there! 😄 How can I assist you today?"
    }]

# Display chat history
st.subheader("Chat History")
for message in st.session_state[f"{PAGE_KEY}_messages"]:
    with st.chat_message(message["role"]):
        st.write(f"{message['content']}")
    st.markdown("---")

# Chat input
if prompt := st.chat_input("Type your message..."):
    st.session_state[f"{PAGE_KEY}_messages"].append({
        "role": "user",
        "content": prompt
    })

    with st.chat_message("user"):
        st.write(prompt)

    with st.chat_message("assistant"):
        with st.spinner("Thinking..."):
            try:
                response = requests.post(
                    server_url,
                    data=json.dumps({"prompt": prompt}),
                    headers={"Content-Type": "application/json"}
                )
                if response.status_code == 200:
                    answer = response.json().get("answer", "No response received.")
                else:
                    answer = f"Error: {response.status_code} {response.text}"
            except Exception as e:
                answer = f"Request failed: {e}"

            st.write(answer)
            st.session_state[f"{PAGE_KEY}_messages"].append({
                "role": "assistant",
                "content": answer
            })
