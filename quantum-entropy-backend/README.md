# Quantum Entropy Backend for llama.cpp

A pluggable entropy abstraction layer for llama.cpp and Ollama that allows researchers to experiment with different entropy sources—including hardware QRNGs and online QRNG services—without modifying the language model or the sampling algorithms themselves.

## Features

- **Pluggable entropy providers**: Philox, ChaCha20, QRNG, Hybrid, File
- **Lock-free ring buffer**: 128MB default, high-performance
- **Background refill thread**: Non-blocking QRNG data fetching
- **LFDR QRNG API integration**: Real quantum random numbers from lfdr.de
- **Fallback modes**: Block, Philox, Throw
- **Runtime statistics**: Bytes downloaded, refill count/latency, starvation, fallback count
- **TOML configuration**: Easy to configure via file or environment variables

## Quick Start

### Build

```bash
cd quantum-entropy-backend
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Run Tests

```bash
ctest --output-on-failure
```

### Benchmark

```bash
./entropy_bench
```

## Integration with llama.cpp

### 1. Copy the library

```bash
cp -r include/entropy /path/to/llama.cpp/include/
cp -r src/*.cpp /path/to/llama.cpp/src/
```

### 2. Apply the patch

```bash
cd /path/to/llama.cpp
git apply /path/to/quantum-entropy-backend/patches/llama-sampler-integration.patch
```

### 3. Update CMakeLists.txt

Add to your llama.cpp CMakeLists.txt:

```cmake
find_package(CURL REQUIRED)
target_link_libraries(llama PUBLIC CURL::libcurl)

# Add entropy sources
list(APPEND LLAMA_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/entropy_provider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/philox_provider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/chacha20_provider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qrng_provider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qrng_http_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hybrid_provider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/file_provider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ring_buffer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config.cpp
)
```

### 4. Build and run

```bash
cd /path/to/llama.cpp
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
./llama-cli -m your-model.gguf
```

## Configuration

### Environment Variables

```bash
export ENTROPY_PROVIDER=qrng
export ENTROPY_QRNG_ENDPOINT=https://lfdr.de/qrng_api
export ENTROPY_QRNG_BUFFER_MB=128
export ENTROPY_QRNG_LOW_WATER_MB=32
export ENTROPY_QRNG_TIMEOUT_MS=5000
export ENTROPY_FALLBACK_MODE=philox
```

### TOML Configuration

Create `entropy.toml`:

```toml
[entropy]
provider = "qrng"

[qrng]
endpoint = "https://lfdr.de/qrng_api"
buffer_mb = 128
low_water_mb = 32
timeout_ms = 5000

[fallback]
mode = "philox"
```

## Providers

### PhiloxProvider
- **Description**: Wraps mt19937_64 (baseline, matches llama.cpp's current RNG)
- **Performance**: ~148M tokens/sec, 6.7ns latency
- **Use case**: Default provider, deterministic

### ChaCha20Provider
- **Description**: Deterministic, seedable CSPRNG
- **Performance**: ~35M tokens/sec, 28.6ns latency
- **Use case**: Reproducible experiments

### QRNGProvider
- **Description**: Real quantum random numbers from LFDR QRNG API
- **Performance**: Depends on network latency
- **Use case**: True randomness for research

### HybridProvider
- **Description**: Periodically reseeds CSPRNG from QRNG
- **Performance**: Between Philox and QRNG
- **Use case**: Balance between speed and true randomness

### FileProvider
- **Description**: Reads entropy from binary file
- **Performance**: Depends on disk speed
- **Use case**: Reproducible testing with known entropy

## Benchmark Results

```
Provider                 Tokens/sec  Avg latency(ns)  Bytes consumed
-----------------------------------------------------------------
Philox (mt19937_64)       148,233,245            6.7        8,000,000
ChaCha20                   35,024,890           28.6        8,000,000
```

## Architecture

```
Prompt
   ↓
Transformer
   ↓
Logits
   ↓
Softmax
   ↓
Sampler
   ↓
EntropyProvider
        │
        ├── PhiloxProvider
        ├── ChaCha20Provider
        ├── QRNGProvider
        │       ↓
        │   Background refill thread
        │       ↓
        │   Ring buffer (128MB)
        │       ↓
        │   LFDR QRNG API
        │
        ├── HybridProvider
        │       ↓
        │   Primary + Secondary
        │
        └── FileProvider
                ↓
            Binary file
```

## License

MIT
