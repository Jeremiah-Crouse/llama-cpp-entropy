#!/bin/bash
set -e

echo "=== Quantum Entropy Backend - Termux Build ==="
echo "Target: aarch64 (Samsung Galaxy A16)"
echo ""

# Install dependencies
echo "[1/6] Installing dependencies..."
pkg update -y
pkg install -y cmake clang git curl openssl python

# Configure
echo "[2/6] Configuring..."
cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DLLAMA_CURL=ON \
  -DLLAMA_OPENMP=OFF \
  -DLLAMA_TESTS=OFF \
  -DLLAMA_BUILD_EXAMPLES=ON \
  -DLLAMA_BUILD_SERVER=ON \
  -DCURL_INCLUDE_DIR=$(pkg-config --cflags-only-I libcurl | sed 's/-I//') \
  -DCURL_LIBRARY=$(pkg-config --libs-only-L libcurl | sed 's/-L//')/libcurl.so

# Build
echo "[3/6] Building (this may take 10-20 minutes)..."
cmake --build build -j$(nproc)

# Verify
echo "[4/6] Verifying build..."
if [ -f build/bin/llama-cli ]; then
  echo "SUCCESS: llama-cli built"
  ./build/bin/llama-cli --version
else
  echo "ERROR: build failed"
  exit 1
fi

# Setup
echo "[5/6] Setting up binaries..."
mkdir -p ~/llama-entropy
cp build/bin/llama-cli ~/llama-entropy/
cp build/bin/llama-server ~/llama-entropy/
cp build/bin/llama-bench ~/llama-entropy/

cat > ~/llama-entropy/run.sh << 'RUNEOF'
#!/bin/bash
export ENTROPY_PROVIDER=qrng
export ENTROPY_QRNG_ENDPOINT=https://lfdr.de/qrng_api

MODEL="${1:-$HOME/models/model.gguf}"
shift 2>/dev/null

~/llama-entropy/llama-cli \
  -m "$MODEL" \
  -ngl 99 \
  --chat
RUNEOF
chmod +x ~/llama-entropy/run.sh

# Setup Telegram harness
echo "[6/6] Setting up Telegram harness..."
pip install requests 2>/dev/null || pip3 install requests 2>/dev/null || true
cp telegram-harness.py ~/llama-entropy/
chmod +x ~/llama-entropy/telegram-harness.py

cat > ~/llama-entropy/start-bot.sh << 'BOTEOF'
#!/bin/bash
# Start the Telegram bot with local LLM
# Set your bot token first:
#   export TELEGRAM_BOT_TOKEN=your_token_here

if [ -z "$TELEGRAM_BOT_TOKEN" ]; then
  echo "Error: Set TELEGRAM_BOT_TOKEN first"
  echo "  export TELEGRAM_BOT_TOKEN=your_token_here"
  exit 1
fi

export LLAMA_URL="http://localhost:8080/v1/chat/completions"
export LLAMA_MODEL="local"
export ENTROPY_PROVIDER=qrng
export ENTROPY_QRNG_ENDPOINT=https://lfdr.de/qrng_api

# Start llama-server in background
echo "Starting llama-server..."
~/llama-entropy/llama-server \
  -m "${1:-$HOME/models/model.gguf}" \
  -ngl 99 \
  --host 0.0.0.0 \
  --port 8080 &
LLAMA_PID=$!
sleep 3

echo "Starting Telegram bot..."
python3 ~/llama-entropy/telegram-harness.py

# Cleanup
kill $LLAMA_PID 2>/dev/null
BOTEOF
chmod +x ~/llama-entropy/start-bot.sh

echo ""
echo "=== Build Complete ==="
echo "Binaries: ~/llama-entropy/"
echo ""
echo "Options:"
echo "  1. Interactive chat: ~/llama-entropy/run.sh /path/to/model.gguf"
echo "  2. Telegram bot:     ~/llama-entropy/start-bot.sh /path/to/model.gguf"
echo ""
echo "For Telegram bot, set your token first:"
echo "  export TELEGRAM_BOT_TOKEN=your_token_here"
echo "  ~/llama-entropy/start-bot.sh ~/models/Qwen2.5-3B-Instruct-Q4_K_M.gguf"
