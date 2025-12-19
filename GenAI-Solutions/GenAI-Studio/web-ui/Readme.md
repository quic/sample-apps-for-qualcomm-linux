
# Web‑UI (Unified Frontend)
## Call Flow

* A browser‑based interface that unifies all GenAI‑Studio services (Text‑to‑Text, TTS, Text‑to‑Image, ASR) behind simple panels.
* Talks to the individual REST endpoints; supports token streaming for chat and progress updates for image/audio generation.
* Provides session handling, prompt history, system‑prompt controls, and sampler parameter inputs.
* Visualizes outputs: chat transcripts, audio players for TTS, image galleries for diffusion results, and ASR transcripts.
* Implements basic error feedback and recovery (e.g., retry after model reload); logs API timing in the console.
* Designed for containerized deployment on Qualcomm; typically runs alongside each service container.
* Uses environment/config to bind to host ports (e.g., 8088 for Text‑to‑Text) on Dragonwing boards.
* Supports multi‑turn chat with KV‑cache continuity and optional role templates (LLAMA formats).
* Extensible for demos like ASR→LLM→TTS pipelines, document summarization.
* Provides a developer‑friendly surface to validate models, parameters, and device backends quickly.

![image_generation](static/assets/genai-studio-workflow.png)

