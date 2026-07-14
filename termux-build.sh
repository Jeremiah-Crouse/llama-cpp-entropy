#!/bin/bash
set -e

echo "=== Quantum Entropy Backend - Termux Build ==="
echo "Target: aarch64 (Samsung Galaxy A16)"
echo ""

# Install dependencies
echo "[1/5] Installing dependencies..."
pkg update -y
pkg install -y cmake clang git curl openssl

# Configure
echo "[2/5] Configuring..."
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
echo "[3/5] Building (this may take 10-20 minutes)..."
cmake --build build -j$(nproc)

# Verify
echo "[4/5] Verifying build..."
if [ -f build/bin/llama-cli ]; then
  echo "SUCCESS: llama-cli built"
  ./build/bin/llama-cli --version
else
  echo "ERROR: build failed"
  exit 1
fi

# Setup
echo "[5/5] Setting up..."
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

echo ""
echo "=== Build Complete ==="
echo "Binaries: ~/llama-entropy/"
echo "To run: ~/llama-entropy/run.sh /path/to/model.gguf"
echo ""
echo "Or manually:"
echo "  ENTROPY_PROVIDER=qrng ENTROPY_QRNG_ENDPOINT=https://lfdr.de/qrng_api \\"
echo "    ~/llama-entropy/llama-cli -m /path/to/model.gguf -ngl 99 --chat"
