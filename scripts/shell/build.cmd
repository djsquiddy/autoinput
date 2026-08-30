@echo off
setlocal
set "CURRENT_DIR=%~dp0"
set "SCRIPT_DIR=%CURRENT_DIR%.."

pushd %SCRIPT_DIR%
    python -m "commands.build" all Release -DCMAKE_C_COMPILER=clang-cl -DCMAKE_CXX_COMPILER=clang-cl
    set "RET=%ERRORLEVEL%"
popd

if %RET% neq 0 (
    endlocal
    exit /b %RET%
)

endlocal
exit /b 0
