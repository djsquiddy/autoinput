@echo off
setlocal
set "CURRENT_DIR=%~dp0"

call "%CURRENT_DIR%\build.cmd" || goto :end

pushd "%CURRENT_DIR%\..\build" || (endlocal & exit /b 1)
    .\bin\autoinput.exe %* || goto :end
popd
endlocal
exit /b 0

:end
    set "RET=%ERRORLEVEL%"
    if %RET% neq 0 (
        endlocal
        exit /b %RET%
    )
    endlocal
    exit /b 0
