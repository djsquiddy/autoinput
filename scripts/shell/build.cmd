@echo off
setlocal
set "CURRENT_DIR=%~dp0"
set "SCRIPT_DIR=%CURRENT_DIR%.."

pushd %SCRIPT_DIR%
    python -m "commands.build" %*
    set "RET=%ERRORLEVEL%"
popd

if %RET% neq 0 (
    endlocal
    exit /b %RET%
)

endlocal
exit /b 0
