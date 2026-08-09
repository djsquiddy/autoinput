@echo off
setlocal
set "CURRENT_DIR=%~dp0"
set "BuildType=%~1"
if "%BuildType%"=="" set "BuildType=Release"
set "BUILD_TESTS=OFF"
set "BUILD_TRAY=OFF"
set "BUILD_UI=OFF"

pushd "%CURRENT_DIR%\.." || exit /b 1

    if not exist build (
        echo creating build directory.
        mkdir build || goto :end
    )

    pushd build || goto :end
        echo Running CMake...
        cmake -G Ninja^
            -DCMAKE_BUILD_TYPE=%BuildType%^
            -DAUTOINPUT_BUILD_TESTS=%BUILD_TESTS%^
            -DAUTOINPUT_BUILD_TRAY=%BUILD_TRAY%^
            -DAUTOINPUT_BUILD_UI=%BUILD_UI%^
            .. || goto :end
        echo Finished running CMake.
        echo Rebuilding...
        ninja || goto :end
        echo Finished rebuilding.
        echo Running tests...
        .\bin\autoinput-tests.exe || goto :end
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
