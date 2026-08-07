@echo off

set "CURRENT_DIR=%~dp0"

set "AUTOINPUT_PATH=%CURRENT_DIR%..\build\bin"
echo adding autoinput to PATH: %AUTOINPUT_PATH%

set "PATH=%AUTOINPUT_PATH%;%PATH%"
goto :eof
