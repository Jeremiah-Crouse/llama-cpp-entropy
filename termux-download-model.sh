#!/bin/bash
set -e

# Download a GGUF model for testing
# Usage: ./download-model.sh [model-name]

MODEL="${1:-tinyllamas}"

case "$MODEL" in
  tinyllamas)
    URL="https://huggingface.co/ggml-org/stories260K/resolve/main/stories260K.gguf"
    NAME="stories260K.gguf"
    ;;
  qwen2.5-3b)
    URL="https://huggingface.co/unsloth/Qwen2.5-3B-Instruct-GGUF/resolve/main/Qwen2.5-3B-Instruct-Q4_K_M.gguf"
    NAME="Qwen2.5-3B-Instruct-Q4_K_M.gguf"
    ;;
  qwen2.5-coder-7b)
    URL="https://huggingface.co/unsloth/Qwen2.5-Coder-7B-Instruct-GGUF/resolve/main/Qwen2.5-Coder-7B-Instruct-Q4_K_M.gguf"
    NAME="Qwen2.5-Coder-7B-Instruct-Q4_K_M.gguf"
    ;;
  *)
    echo "Unknown model: $MODEL"
    echo "Available: tinyllamas, qwen2.5-3b, qwen2.5-coder-7b"
    exit 1
    ;;
esac

mkdir -p ~/models
echo "Downloading $NAME..."
curl -L -o ~/models/"$NAME" "$URL"
echo "Saved to ~/models/$NAME"
echo ""
echo "Run with:"
echo "  ~/llama-entropy/run.sh ~/models/$NAME"
