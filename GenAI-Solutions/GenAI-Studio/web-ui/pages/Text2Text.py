
# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------

import streamlit as st
import requests
import json

# ---------------------------
# Page Config
# ---------------------------
st.set_page_config(page_title="GenAI Studio", layout="wide")
PAGE_KEY = "text2text"

# ---------------------------
# Server URLs (adjust as needed)
# ---------------------------
SERVER_PROCESS_URL = 'http://0.0.0.0:8088/process'
SERVER_RESET_URL = 'http://0.0.0.0:8088/reset_model'
SERVER_RELOAD_URL = 'http://0.0.0.0:8088/reload_model'

# ---------------------------
# Helper functions
# ---------------------------
def call_backend(url: str, payload: dict | None = None) -> tuple[bool, str]:
    """Call a backend endpoint and return (success, message)."""
    try:
        if payload is None:
            response = requests.post(url)
        else:
            response = requests.post(url, data=json.dumps(payload), headers={"Content-Type": "application/json"})

        if response.status_code == 200:
            try:
                data = response.json()
                print(f"_39_data:{data}")
                msg = data.get("message") or data.get("status") or "Success."
            except Exception:
                msg = response.text or "Success."
            return True, msg
        else:
            return False, f"Error: {response.status_code} {response.text}"
    except Exception as e:
        return False, f"Request failed: {e}"

def notify_success(msg: str):
    if hasattr(st, "toast"):
        st.toast(msg, icon="✅")
    else:
        st.success(msg)

def notify_error(msg: str):
    if hasattr(st, "toast"):
        st.toast(msg, icon="❌")
    else:
        st.error(msg)


def reset_conversation():
    """
    Reseting the conversation till now
    """
    st.session_state[f"{PAGE_KEY}_messages"] = [{
            "role": "assistant",
            "content": "Hey there! 😄 How can I assist you today?"
        }]
    st.rerun()

def reset_model():
    """Trigger model reset on backend."""
    success, msg = call_backend(SERVER_RESET_URL)
    if success:
        st.success(f"✅ Model reset successful: {msg}")
        reset_conversation()
        notify_success(f"✅ Model reset successful: {msg}")
    else:
        st.error(f"❌ Model reload failed: {msg}")
        notify_error(f"❌ Model reload failed: {msg}")

def reload_model():
    """Trigger model reload on backend."""
    success, msg = call_backend(SERVER_RELOAD_URL)
    if success:
        st.success(f"✅ Model reload successful: {msg}")
        reset_conversation()
        notify_success(f"✅ Model reload successful: {msg}")
    else:
        st.error(f"❌ Model reload failed: {msg}")
        notify_error(f"❌ Model reload failed: {msg}")

# ---------------------------
# Sidebar
# ---------------------------
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




# ---------------------------
# Top Control Bar (Right-Aligned Buttons)
# ---------------------------
# Create a row with columns: title on the left, buttons on the right
top_left, top_spacer, top_right = st.columns([0.3, 0.3, 0.4])
with top_left:
    st.markdown("## GenAI Studio")

with top_right:
    # Place buttons side-by-side on the top-right
    btn_cols = st.columns(2)
    with btn_cols[0]:
        if st.button("🔄 Reset Conversation", use_container_width=True):
            reset_model()
    with btn_cols[1]:
        if st.button("♻️ Reload Model", use_container_width=True):
            reload_model()

st.markdown("---")
# ---------------------------
# Initialize page-specific chat history
# ---------------------------
if f"{PAGE_KEY}_messages" not in st.session_state:
    st.session_state[f"{PAGE_KEY}_messages"] = [{
        "role": "assistant",
        "content": "Hey there! 😄 How can I assist you today?"
    }]

# ---------------------------
# Chat History Display
# ---------------------------
st.subheader("Chat History")
for message in st.session_state[f"{PAGE_KEY}_messages"]:
    with st.chat_message(message["role"]):
        st.write(f"{message['content']}")
    st.markdown("---")

# ---------------------------
# Chat Input & Server Interaction
# ---------------------------
if prompt := st.chat_input("Type your message..."):
    st.session_state[f"{PAGE_KEY}_messages"].append({
        "role": "user",
        "content": prompt
    })

    with st.chat_message("user"):
        st.write(prompt)

    with st.chat_message("assistant"):
        with st.spinner("Thinking..."):
            success, result_msg = call_backend(
                SERVER_PROCESS_URL,
                payload={"prompt": prompt}
            )
            # If backend returns JSON with "answer", prefer that
            answer = result_msg
            try:
                # Attempt to parse answer from JSON if possible
                response = requests.post(
                    SERVER_PROCESS_URL,
                    data=json.dumps({"prompt": prompt}),
                    headers={"Content-Type": "application/json"}
                )
                if response.status_code == 200:
                    answer_json = response.json()
                    answer = answer_json.get("answer", answer)
                else:
                    answer = f"Error: {response.status_code} {response.text}"
            except Exception as e:
                # Fall back to previous msg if secondary call fails
                if not success:
                    answer = f"Request failed: {e}"

            st.write(answer)
            st.session_state[f"{PAGE_KEY}_messages"].append({
                "role": "assistant",
                "content": answer
            })
