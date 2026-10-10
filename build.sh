#!/usr/bin/env bash
set -e

echo "====================================================="
echo "  RELIEF-Q — C to WebAssembly Compilation Script     "
echo "====================================================="

if command -v emcc &> /dev/null; then
    echo "[OK] Found emcc in PATH"
    mkdir -p wasm
    emcc c_src/data_structures.c c_src/simulator.c c_src/json_utils.c c_src/wasm_api.c \
      -O3 \
      -s WASM=1 \
      -s ALLOW_MEMORY_GROWTH=1 \
      -s MODULARIZE=1 \
      -s EXPORT_NAME="createReliefQModule" \
      -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap','UTF8ToString']" \
      -o wasm/relief_q.js
    echo "[OK] Generated wasm/relief_q.js and wasm/relief_q.wasm"
else
    echo "[INFO] emcc not found. Checking if precompiled wasm exists..."
    if [ -f "wasm/relief_q.wasm" ]; then
        echo "[OK] Pre-compiled WebAssembly binary found: wasm/relief_q.wasm"
    else
        echo "[ERROR] WebAssembly binary missing and emcc not found!"
        exit 1
    fi
fi

if command -v node &> /dev/null; then
    node scripts/build.js
fi

echo "====================================================="
echo "  Build successful! Ready for Vercel deployment."
echo "====================================================="
