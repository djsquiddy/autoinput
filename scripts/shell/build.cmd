@echo off
setlocal
set "CURRENT_DIR=%~dp0"
set "PROJECT_DIR=%CURRENT_DIR%.."

pushd %PROJECT_DIR%
    python -m "scripts.build" %*
    set "RET=%ERRORLEVEL%"
popd

if %RET% neq 0 (
    endlocal
    exit /b %RET%
)

endlocal
exit /b 0
