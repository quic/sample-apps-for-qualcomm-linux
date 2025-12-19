
# ---------------------------------------------------------------------
# Copyright (c) Qualcomm Innovation Center, Inc. All rights reserved.
# SPDX-License-Identifier: BSD-3-Clause
# ---------------------------------------------------------------------

import streamlit as st
import requests
import json
from dataclasses import dataclass
import time
import hashlib
import json
# simple_logger.py
import time
from typing import Any, Dict, List, Optional

# ---------------------------
# Logger Part
# ---------------------------
@dataclass
class LogEntry:
    timestamp: str
    level: str
    message: str
    elapsed_ms: Optional[float] = None
    extra: Optional[Dict[str, Any]] = None


class SimpleLogger:
    def __init__(self):
        self.logs: List[LogEntry] = []

    def log(self, message: str, level: str = "INFO", extra: Dict[str, Any] | None = None, elapsed_ms: float | None = None):
        ts = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
        self.logs.append(LogEntry(timestamp=ts, level=level.upper(), message=message, elapsed_ms=elapsed_ms, extra=extra or {}))

    # Context manager to time a block and auto-log start/end with elapsed time
    def time_block(self, message: str, level: str = "INFO", extra_start: Dict[str, Any] | None = None, extra_end: Dict[str, Any] | None = None):
        class _TimerCtx:
            def __init__(self, outer: SimpleLogger, msg: str, lvl: str, xstart: Dict[str, Any] | None, xend: Dict[str, Any] | None):
                self.outer = outer
                self.msg = msg
                self.level = lvl
                self.extra_start = xstart or {}
                self.extra_end = xend or {}
            def __enter__(self):
                self.start = time.perf_counter()
                self.outer.log(f"{self.msg} - start", level=self.level, extra=self.extra_start)
                return self
            def __exit__(self, exc_type, exc, tb):
                end = time.perf_counter()
                elapsed_ms = (end - self.start) * 1000.0
                status = "SUCCESS" if exc is None else "ERROR"
                self.outer.log(f"{self.msg} - end ({status})", level=self.level, extra=self.extra_end, elapsed_ms=elapsed_ms)
                # Do not suppress exceptions
                return False
        return _TimerCtx(self, message, level, extra_start, extra_end)

    def get_logs(self) -> List[Dict[str, Any]]:
        # Return serializable dicts
        from dataclasses import asdict
        return [asdict(e) for e in self.logs]

    def clear(self):
        self.logs.clear()

    def export_as_json(self) -> str:
        import json
        return json.dumps(self.get_logs(), indent=2)




# ---------------------------
# Page Config
# ---------------------------
st.set_page_config(page_title="GenAI Studio", layout="wide")
PAGE_KEY = "text2text"

if "logger" not in st.session_state:
    st.session_state.logger = SimpleLogger()
logger = st.session_state.logger

@dataclass
class GenerationSettings:
    system_prompt: str = "You're an helpfull AI assistant"
    seed: int = 42
    temp: float = 0.8
    top_k: int = 40
    top_p: float = 0.95
    max_tokens:int=200

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

# ---------------------------
# Sidebar
# ---------------------------

# Initialize settings
if "gen_settings" not in st.session_state:
    st.session_state.gen_settings = GenerationSettings()

def _hash_payload(payload: dict) -> str:
    """Deterministic hash of the payload to detect unchanged settings."""
    return hashlib.sha256(json.dumps(payload, sort_keys=True).encode("utf-8")).hexdigest()

def reload_model():
    """Trigger model reload on backend with updated settings."""
    settings = st.session_state.gen_settings
    logger: SimpleLogger = st.session_state.logger

    greedy_str = str(bool(settings.greedy)).lower()
    sampler_block = f"""
    {{
        "sampler": {{
            "version": 1,
            "type": "basic",
            "seed": {settings.seed},
            "temp": {settings.temp},
            "top-k": {settings.top_k},
            "top-p": {settings.top_p},
            "greedy": {greedy_str}
        }}
    }}
    """.strip()

    payload = {
        "system_prompt": settings.system_prompt,
        "max_tokens": settings.max_tokens,
        "sampler_block": sampler_block
    }

    # Skip reloading if settings haven't changed
    try:
        new_hash = _hash_payload(payload)
        prev_hash = st.session_state.get("last_reload_hash")
        if prev_hash == new_hash:
            logger.log("Reload skipped: settings unchanged", level="INFO", extra={"route": "/reload_model"})
            notify_success("Reload skipped — settings unchanged.")
            return
        st.session_state["last_reload_hash"] = new_hash
    except Exception as e:
        logger.log("Reload hash check failed", level="WARN", extra={"error": str(e)})

    # Timing via context manager
    with logger.time_block("Model reload", level="INFO",
                           extra_start={"route": "/reload_model",
                                        "system_prompt_len": len(settings.system_prompt),
                                        "sampler_block_len": len(sampler_block)},
                           extra_end={"route": "/reload_model"}):
        st.write("📤 Sending settings to server:", payload)

        start = time.perf_counter()
        success, msg = call_backend(SERVER_RELOAD_URL, payload)
        end = time.perf_counter()
        elapsed_ms = (end - start) * 1000.0

        # Detailed log of backend call
        logger.log(
            "Backend reload call outcome",
            level="INFO",
            elapsed_ms=elapsed_ms,
            extra={
                "success": success,
                "message": msg,
                "payload_preview": {
                    "system_prompt": (settings.system_prompt[:256] + "…") if len(settings.system_prompt) > 256 else settings.system_prompt,
                    "seed": settings.seed,
                    "temp": settings.temp,
                    "top_k": settings.top_k,
                    "top_p": settings.top_p,
                    "greedy": settings.greedy,
                    "max_tokens": settings.max_tokens,
                }
            }
        )

        if success:
            notify_success(f"✅ Model reload successful: {msg}")
            reset_conversation()
        else:
            notify_error(f"❌ Model reload failed: {msg}")


def reset_model():
    """Trigger model reset on backend with timing and structured logs."""
    logger: SimpleLogger = st.session_state.logger

    # Log the operation and measure its elapsed time
    with logger.time_block(
        "Model reset",
        level="INFO",
        extra_start={"route": "/reset_model"},
        extra_end={"route": "/reset_model"}
    ):
        # Measure the backend call precisely
        t0 = time.perf_counter()
        success, msg = call_backend(SERVER_RESET_URL)
        t1 = time.perf_counter()
        elapsed_ms = (t1 - t0) * 1000.0

        # Record detailed outcome
        logger.log(
            "Backend reset call outcome",
            level="INFO",
            elapsed_ms=elapsed_ms,
            extra={
                "success": success,
                "message": msg
            }
        )

        # UI feedback + conversation state handling
        if success:
            st.success(f"✅ Model reset successful: {msg}")
            reset_conversation()
            notify_success(f"✅ Model reset successful: {msg}")
        else:
            st.error(f"❌ Model reset failed: {msg}")
            notify_error(f"❌ Model reset failed: {msg}")

st.subheader("Client Logs")
logs = logger.get_logs()

with st.expander(f"Recent Logs (count={len(logs)})", expanded=False):
    for i, e in enumerate(logs[-50:], 1):
        st.markdown(f"**[{i}] {e['timestamp']} — {e['level']} — {e['message']}**")
        if e.get("elapsed_ms") is not None:
            st.write(f"Elapsed: {e['elapsed_ms']:.1f} ms")
        if e.get("extra"):
            st.code(json.dumps(e["extra"], indent=2))

if logs:
    st.download_button(
        "📥 Download Logs (JSON)",
        data=logger.export_as_json(),
        file_name="client_logs.json",
        mime="application/json",
        use_container_width=True
    )



with st.sidebar:
    st.title("Generation Settings")
    # System Prompt
    st.session_state.gen_settings.system_prompt = st.text_area(
        "System Prompt", st.session_state.gen_settings.system_prompt
    )
    st.session_state.gen_settings.max_tokens = st.number_input(
        "max_tokens", value=st.session_state.gen_settings.max_tokens, step=10
    )
    # Seed
    st.session_state.gen_settings.seed = st.number_input(
        "Seed", value=st.session_state.gen_settings.seed, step=1
    )
    # Temperature
    st.session_state.gen_settings.temp = st.slider(
        "Temperature", 0.0, 1.0, st.session_state.gen_settings.temp
    )
    # Top-K
    st.session_state.gen_settings.top_k = st.number_input(
        "Top-K", value=st.session_state.gen_settings.top_k, step=1
    )
    # Top-P
    st.session_state.gen_settings.top_p = st.slider(
        "Top-P", 0.0, 1.0, st.session_state.gen_settings.top_p
    )
    # ✅ Greedy Mode Toggle
    st.session_state.gen_settings.greedy = st.checkbox(
        "Enable Greedy Decoding", value=getattr(st.session_state.gen_settings, "greedy", False)
    )
    # Reload Model Button
    if st.button("♻️ Reload Model"):
        reload_model()

    if st.button("🔄 Reset Conversation"):
        reset_model()


# reload_model()
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


settings = st.session_state.gen_settings

# (Optional) request counter for unique IDs
if "req_counter" not in st.session_state:
    st.session_state.req_counter = 0


if prompt := st.chat_input("Type your message..."):
    # 1) Append USER message (mutate the existing list)
    st.session_state[f"{PAGE_KEY}_messages"].append({"role": "user", "content": prompt})

    # 2) Show user message in UI immediately
    with st.chat_message("user"):
        st.write(prompt)

    # 3) Prepare identifiers & sampler snapshot for logging (safe truncation)
    st.session_state.req_counter += 1
    req_id = f"process-{st.session_state.req_counter}"

    def _short(s: str, limit: int = 2048) -> str:
        return s if s is None or len(s) <= limit else (s[:limit] + "…")

    greedy_str = str(bool(getattr(settings, "greedy", False))).lower()
    sampler_block = f"""
    {{
        "sampler": {{
            "version": 1,
            "type": "basic",
            "seed": {getattr(settings, "seed", 42)},
            "temp": {getattr(settings, "temp", 0.8)},
            "top-k": {getattr(settings, "top_k", 40)},
            "top-p": {getattr(settings, "top_p", 0.95)},
            "greedy": {greedy_str}
        }}
    }}
    """.strip()

    # 4) Query backend & append ASSISTANT response
    with st.chat_message("assistant"):
        with st.spinner("Thinking..."):
            t0 = time.perf_counter()
            answer = None
            error = None
            http_status = None

            try:
                resp = requests.post(
                    SERVER_PROCESS_URL,
                    data=json.dumps({"prompt": prompt}),
                    headers={"Content-Type": "application/json"},
                    timeout=60  # helpful timeout
                )
                http_status = resp.status_code
                if resp.status_code == 200:
                    answer_json = resp.json()
                    answer = answer_json.get("answer", "No answer from LLM")
                else:
                    error = f"Error: {resp.status_code} {resp.text}"
                    answer = error
            except Exception as e:
                error = f"Request failed: {e}"
                answer = error

            t1 = time.perf_counter()
            elapsed_ms = (t1 - t0) * 1000.0

            # Show assistant answer
            st.write(answer)

            # ✅ Append ASSISTANT message (mutate, not reassign)
            st.session_state[f"{PAGE_KEY}_messages"].append({
                "role": "assistant",
                "content": answer
            })

            # Optional: persist chat snapshot
            # save_chat_to_jsonl(st.session_state[CHAT_KEY])

            # 5) Log outcome
            logger.log(
                f"[{req_id}] /process",
                level="INFO",
                elapsed_ms=elapsed_ms,
                extra={
                    "status": "success" if error is None else "failure",
                    "http_status": http_status,
                    "user_prompt": _short(prompt),
                    "answer": _short(answer, 4096),
                    "system_prompt": _short(getattr(settings, "system_prompt", ""), 2048),
                    "sampler_block": _short(sampler_block, 4096),
                    "settings": {
                        "seed": getattr(settings, "seed", 42),
                        "temp": getattr(settings, "temp", 0.8),
                        "top_k": getattr(settings, "top_k", 40),
                        "top_p": getattr(settings, "top_p", 0.95),
                        "greedy": getattr(settings, "greedy", False),
                        "max_tokens": getattr(settings, "max_tokens", 200),
                    }
                }
            )
