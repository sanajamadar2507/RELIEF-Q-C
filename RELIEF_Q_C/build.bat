@echo off
setlocal enabledelayedexpansion

echo =====================================================
echo   RELIEF-Q — C to WebAssembly Compilation Script
echo =====================================================

:: Check if emcc is already in PATH
where emcc >nul 2>nul
if %ERRORLEVEL% EQU 0 (
    echo [OK] Found emcc in PATH
    goto :COMPILE
)

:: Check known EMSDK locations
if exist "C:\Users\sanaj\emsdk\emsdk_env.bat" (
    echo [INFO] Activating Emscripten from C:\Users\sanaj\emsdk...
    call "C:\Users\sanaj\emsdk\emsdk_env.bat" >nul 2>nul
    goto :COMPILE
)

if exist "%USERPROFILE%\emsdk\emsdk_env.bat" (
    echo [INFO] Activating Emscripten from %USERPROFILE%\emsdk...
    call "%USERPROFILE%\emsdk\emsdk_env.bat" >nul 2>nul
    goto :COMPILE
)

if exist "C:\emsdk\emsdk_env.bat" (
    echo [INFO] Activating Emscripten from C:\emsdk...
    call "C:\emsdk\emsdk_env.bat" >nul 2>nul
    goto :COMPILE
)

echo [WARNING] Emscripten SDK not found in PATH or standard folders.
echo If pre-compiled wasm/relief_q.wasm already exists, you can deploy directly.
echo To compile from source, install emsdk as described in the README.
goto :VERIFY

:COMPILE
if not exist "wasm" mkdir wasm

echo [INFO] Compiling C source files to WebAssembly (O3 optimization)...
emcc c_src/data_structures.c c_src/simulator.c c_src/json_utils.c c_src/wasm_api.c ^
  -O3 ^
  -s WASM=1 ^
  -s ALLOW_MEMORY_GROWTH=1 ^
  -s MODULARIZE=1 ^
  -s EXPORT_NAME="createReliefQModule" ^
  -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap','UTF8ToString']" ^
  -o wasm/relief_q.js

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Emscripten compilation failed!
    exit /b %ERRORLEVEL%
)

echo [OK] WebAssembly compilation succeeded:
dir wasm\relief_q.* /B

:VERIFY
echo [INFO] Running production packaging into /dist...
node scripts/build.js

echo =====================================================
echo   Build complete! Run 'python server.py' to test locally.
echo =====================================================
