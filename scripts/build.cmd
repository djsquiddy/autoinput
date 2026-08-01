@echo off
setlocal
set "CURRENT_DIR=%~dp0"
set "BuildType=%~1"
if "%BuildType%"=="" set "BuildType=Release"

pushd "%CURRENT_DIR%\.." || exit /b 1

    if not exist build (
        echo creating build directory.
        mkdir build || goto :end
    )

    pushd build || goto :end
        echo Running CMake...
        cmake -G Ninja -DCMAKE_BUILD_TYPE=%BuildType% .. || goto :end
        echo Finished running CMake.
        echo Rebuilding...
        ninja || goto :end
        echo Finished rebuilding.
        echo Running tests...
        .\bin\autoinput_tests.exe || goto :end
        echo Finished running tests.
    popd
popd

:end
    set "RET=%ERRORLEVEL%"
    if %RET% neq 0 (
        endlocal
        exit /b %RET%
    )
    endlocal
    exit /b 0
