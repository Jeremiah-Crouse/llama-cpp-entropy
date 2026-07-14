#!/usr/bin/env python3
"""
Telegram harness for locally-run LLM via llama-server.
Uses only Python stdlib - no pip install needed.

Usage:
  TELEGRAM_BOT_TOKEN=your_token python3 telegram-harness.py
"""

import os
import sys
import json
import time
import logging
import threading
import urllib.request
import urllib.error
from collections import defaultdict
from pathlib import Path

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%H:%M:%S",
)
log = logging.getLogger("harness")

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

TELEGRAM_API = f"https://api.telegram.org/bot{BOT_TOKEN}"

offset = 0
chat_histories = defaultdict(list)
lock = threading.Lock()


def api_request(url, data=None, timeout=10):
    """Make an HTTP request using only urllib."""
    if data is not None:
        body = json.dumps(data).encode("utf-8")
        req = urllib.request.Request(url, data=body, headers={"Content-Type": "application/json"})
    else:
        req = urllib.request.Request(url)
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8", errors="replace")
        log.error(f"HTTP {e.code}: {body[:200]}")
        return None
    except Exception as e:
        log.error(f"Request error: {e}")
        return None


def send_telegram(chat_id, text):
    url = f"{TELEGRAM_API}/sendMessage"
    api_request(url, {"chat_id": chat_id, "text": text})


def send_typing(chat_id):
    url = f"{TELEGRAM_API}/sendChatAction"
    api_request(url, {"chat_id": chat_id, "action": "typing"})


def call_llama(messages):
    payload = {
        "model": MODEL,
        "messages": [{"role": "system", "content": SYSTEM_PROMPT}] + messages,
        "temperature": 0.7,
        "max_tokens": 2048,
    }
    try:
        result = api_request(LLAMA_URL, payload, timeout=120)
        if result and "choices" in result:
            return result["choices"][0]["message"]["content"]
        return "Error: No response from model"
    except Exception as e:
        return f"Error: {e}"


def get_history(chat_id):
    return chat_histories[chat_id]


def add_to_history(chat_id, role, content):
    chat_histories[chat_id].append({"role": role, "content": content})
    if len(chat_histories[chat_id]) > MAX_HISTORY:
        chat_histories[chat_id] = chat_histories[chat_id][-MAX_HISTORY:]


def handle_command(chat_id, text):
    if text == "/start":
        send_telegram(chat_id, "Local LLM Bot\n\nRunning locally on this phone via Termux with quantum entropy.\n\nCommands:\n/start - This message\n/clear - Clear history\n/model - Show model info")
    elif text == "/clear":
        with lock:
            chat_histories[chat_id] = []
        send_telegram(chat_id, "History cleared.")
    elif text == "/model":
        send_telegram(chat_id, f"Model: {MODEL}\nServer: {LLAMA_URL}")
    return True


def process_message(chat_id, text, user_name):
    log.info(f"[{chat_id}] {user_name}: {text}")
    if text.startswith("/"):
        handle_command(chat_id, text)
        return
    send_typing(chat_id)
    add_to_history(chat_id, "user", text)
    history = get_history(chat_id)
    response = call_llama(history)
    add_to_history(chat_id, "assistant", response)
    log.info(f"[{chat_id}] Bot: {response[:100]}...")
    send_telegram(chat_id, response)


def poll_loop():
    global offset
    if STATE_FILE.exists():
        try:
            offset = int(STATE_FILE.read_text().strip())
            log.info(f"Loaded offset: {offset}")
        except Exception:
            pass
    log.info("Starting poll loop...")
    while True:
        try:
            url = f"{TELEGRAM_API}/getUpdates?offset={offset}&timeout={POLL_TIMEOUT}&allowed_updates=[\"message\"]"
            data = api_request(url, timeout=POLL_TIMEOUT + 10)
            if not data or not data.get("ok"):
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
                user_name = msg.get("from", {}).get("first_name", "Unknown")
                if not text:
                    continue
                t = threading.Thread(target=process_message, args=(chat_id, text, user_name), daemon=True)
                t.start()
            STATE_FILE.write_text(str(offset))
        except KeyboardInterrupt:
            log.info("Shutting down...")
            break
        except Exception as e:
            log.error(f"Poll error: {e}")
            time.sleep(5)


if __name__ == "__main__":
    if not BOT_TOKEN:
        print("Error: Set TELEGRAM_BOT_TOKEN environment variable")
        print("  export TELEGRAM_BOT_TOKEN=your_token_here")
        sys.exit(1)
    log.info(f"Bot token: {BOT_TOKEN[:10]}...")
    log.info(f"LLM server: {LLAMA_URL}")
    log.info(f"Model: {MODEL}")
    poll_loop()
