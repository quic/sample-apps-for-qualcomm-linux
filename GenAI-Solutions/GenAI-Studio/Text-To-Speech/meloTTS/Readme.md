
# Text‑to‑Speech 

## Call Flow

1. Users enters a prompt to be spoken out load (e.g., “read the answer aloud”)
2. The Web‑UI sends the text to the Text‑to‑Speech container over REST API.
3. The TTS service performs speech synthesis and returns an audio stream.Passes this audio back to the web page.
4. Web‑UI: The web front‑end plays the synthesized speech to the user via the browser.

![tts](readme_assets/text2speech_unified_diagram.png)



