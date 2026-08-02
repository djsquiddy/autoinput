@echo off
setlocal
pushd "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File ".\install.ps1"
if %ERRORLEVEL% neq 0 (
    echo.
    echo Installation failed.
    pause
    popd
    exit /b %ERRORLEVEL%
)
echo.
pause
popd
endlocal
exit /b 0
