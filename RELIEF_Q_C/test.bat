@echo off
echo =====================================================
echo   Compiling and Running RELIEF-Q C Test Suite
echo =====================================================
gcc -Wall -Wextra -O2 -I c_src c_src/data_structures.c c_src/simulator.c c_src/json_utils.c tests/test_relief_q.c -o tests/test_relief_q.exe
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] C Test compilation failed!
    exit /b %ERRORLEVEL%
)
echo Running C tests...
tests\test_relief_q.exe
