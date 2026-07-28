@echo off
setlocal
set "CURRENT_DIR=%~dp0"

call "%CURRENT_DIR%\build.cmd" || goto :end

pushd "%CURRENT_DIR%\..\build"
    .\autoinput %* || goto: end
popd

:end
    endlocal
    goto :EOF
