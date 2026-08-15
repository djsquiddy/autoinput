@echo off
setlocal
set "CURRENT_DIR=%~dp0"

python "%CURRENT_DIR%build.py" %*
set "RET=%ERRORLEVEL%"
if %RET% neq 0 (
    endlocal
    exit /b %RET%
)
endlocal
exit /b 0
