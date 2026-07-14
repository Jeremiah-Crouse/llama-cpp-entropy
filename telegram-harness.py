#!/usr/bin/env python3
"""
Telegram harness for locally-run LLM via llama-server.
Polls Telegram, sends to llama-server HTTP API, replies back.

Usage:
  TELEGRAM_BOT_TOKEN=your_token python3 telegram_harness.py

Requires:
  pip install requests
"""

import os
import sys
import json
import time
import logging
import threading
import requests
from collections import defaultdict
from pathlib import Path

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("harness")

# --- Config ---
BOT_TOKEN = os.environ.get("TELEGRAM_BOT_TOKEN", "")
LLAMA_URL = os.environ.get("LLAMA_URL", "http://localhost:8080/v1/chat/completions")
MODEL = os.environ.get("LLAMA_MODEL", "local")
POLL_TIMEOUT = int(os.environ.get("POLL_TIMEOUT", "30"))
SYSTEM_PROMPT = os.environ.get(
    "SYSTEM_PROMPT",
    "You are a helpful AI assistant running locally on a phone via Termux. "
    "You have access to quantum random numbers for enhanced creativity. "
    "Be concise and helpful.",
)
MAX_HISTORY = int(os.environ.get("MAX_HISTORY", "20"))
STATE_FILE = Path(os.environ.get("STATE_FILE", "/tmp/telegram_harness_offset.json"))
HISTORY_DIR = Path(os.environ.get("HISTORY_DIR", "/tmp/telegram_harness_history"))

TELEGRAM_API = f"https://api.telegram.org/bot{BOT_TOKEN}"

# --- State ---
offset = 0
chat_histories = defaultdict(list)
lock = threading.Lock()


def send_telegram(chat_id: int, text: str):
    """Send a message to Telegram."""
    url = f"{TELEGRAM_API}/sendMessage"
    payload = {"chat_id": chat_id, "text": text, "parse_mode": "Markdown"}
    try:
        resp = requests.post(url, json=payload, timeout=10)
        if resp.status_code != 200:
            log.error(f"Telegram send failed: {resp.status_code} {resp.text}")
    except Exception as e:
        log.error(f"Telegram send error: {e}")


def send_typing(chat_id: int):
    """Send typing indicator."""
    url = f"{TELEGRAM_API}/sendChatAction"
    try:
        requests.post(url, json={"chat_id": chat_id, "action": "typing"}, timeout=5)
    except Exception:
        pass


def call_llama(messages: list) -> str:
    """Send messages to llama-server and get response."""
    payload = {
        "model": MODEL,
        "messages": [
            {"role": "system", "content": SYSTEM_PROMPT},
            *messages,
        ],
        "temperature": 0.7,
        "max_tokens": 2048,
        "stream": False,
    }
    try:
        resp = requests.post(LLAMA_URL, json=payload, timeout=120)
        if resp.status_code != 200:
            log.error(f"llama-server error: {resp.status_code} {resp.text}")
            return f"Error: llama-server returned {resp.status_code}"
        data = resp.json()
        return data["choices"][0]["message"]["content"]
    except requests.exceptions.Timeout:
        return "Error: Request timed out (120s). Model may be too slow."
    except Exception as e:
        return f"Error: {e}"


def get_history(chat_id: int) -> list:
    """Get conversation history for a chat."""
    return chat_histories[chat_id]


def add_to_history(chat_id: int, role: str, content: str):
    """Add a message to chat history."""
    chat_histories[chat_id].append({"role": role, "content": content})
    # Trim to max history
    if len(chat_histories[chat_id]) > MAX_HISTORY:
        chat_histories[chat_id] = chat_histories[chat_id][-MAX_HISTORY:]


def handle_command(chat_id: int, text: str):
    """Handle slash commands."""
    if text == "/start":
        send_telegram(
            chat_id,
            "🤖 *Local LLM Bot*\n\n"
            "I'm running locally on this phone via Termux with quantum entropy.\n\n"
            "Commands:\n"
            "/start - This message\n"
            "/clear - Clear conversation history\n"
            "/model - Show current model info\n"
            "/help - Show help",
        )
    elif text == "/clear":
        with lock:
            chat_histories[chat_id] = []
        send_telegram(chat_id, "✅ Conversation history cleared.")
    elif text == "/model":
        send_telegram(chat_id, f"📦 Model: `{MODEL}`\n🔗 Server: `{LLAMA_URL}`")
    elif text == "/help":
        send_telegram(
            chat_id,
            "Just send me a message and I'll respond using the local LLM.\n\n"
            "The model runs entirely on this device. "
            "Quantum random numbers are used for enhanced creativity.",
        )
    return True
    return False


def process_message(chat_id: int, text: str, user_name: str):
    """Process a single message."""
    log.info(f"[{chat_id}] {user_name}: {text}")

    # Handle commands
    if text.startswith("/"):
        if handle_command(chat_id, text):
            return

    # Send typing indicator
    send_typing(chat_id)

    # Add user message to history
    add_to_history(chat_id, "user", text)

    # Get response from llama
    history = get_history(chat_id)
    response = call_llama(history)

    # Add assistant response to history
    add_to_history(chat_id, "assistant", response)

    # Send response
    log.info(f"[{chat_id}] Bot: {response[:100]}...")
    send_telegram(chat_id, response)


def poll_loop():
    """Main polling loop."""
    global offset

    # Load saved offset
    if STATE_FILE.exists():
        try:
            offset = int(STATE_FILE.read_text().strip())
            log.info(f"Loaded offset: {offset}")
        except Exception:
            pass

    log.info("Starting Telegram poll loop...")

    while True:
        try:
            url = f"{TELEGRAM_API}/getUpdates"
            params = {
                "offset": offset,
                "timeout": POLL_TIMEOUT,
                "allowed_updates": ["message"],
            }
            resp = requests.get(url, params=params, timeout=POLL_TIMEOUT + 10)
            data = resp.json()

            if not data.get("ok"):
                log.error(f"Poll error: {data}")
                time.sleep(5)
                continue

            for update in data.get("result", []):
                offset = update["update_id"] + 1
                msg = update.get("message")
                if not msg:
                    continue

                chat_id = msg["chat"]["id"]
                text = msg.get("text", "")
                user = msg.get("from", {})
                user_name = user.get("first_name", "Unknown")

                if not text:
                    continue

                # Process in a thread
                t = threading.Thread(
                    target=process_message,
                    args=(chat_id, text, user_name),
                    daemon=True,
                )
                t.start()

            # Save offset
            STATE_FILE.write_text(str(offset))

        except KeyboardInterrupt:
            log.info("Shutting down...")
            break
        except Exception as e:
            log.error(f"Poll loop error: {e}")
            time.sleep(5)


if __name__ == "__main__":
    if not BOT_TOKEN:
        print("Error: Set TELEGRAM_BOT_TOKEN environment variable")
        print("  export TELEGRAM_BOT_TOKEN=your_token_here")
        sys.exit(1)

    log.info(f"Bot token: {BOT_TOKEN[:10]}...")
    log.info(f"LLM server: {LLAMA_URL}")
    log.info(f"Model: {MODEL}")

    HISTORY_DIR.mkdir(exist_ok=True)
    poll_loop()
