@echo on
setlocal
set "CURRENT_DIR=%~dp0"
set "BuildType=Release"
set "CLEAN=false"

set "BUILD_TESTS=OFF"
set "BUILD_TRAY=OFF"
set "BUILD_UI=OFF"
set "FOUND_ALL_BUILD_TARGET=false"
set "FOUND_BUILD_TARGET=false"

:parse_args
if "%~1"=="" goto :args_done
if /I "%~1"=="--clean" (
    set "CLEAN=true"
) else if /I "%~1"=="/clean" (
    set "CLEAN=true"
) else if /I "%~1"=="ui" (
    set "BUILD_UI=ON"
    set "FOUND_BUILD_TARGET=true"
) else if /I "%~1"=="tray" (
    set "BUILD_TRAY=ON"
    set "FOUND_BUILD_TARGET=true"
) else if /I "%~1"=="tests" (
    set "BUILD_TESTS=ON"
    set "FOUND_BUILD_TARGET=true"
) else if /I "%~1"=="all" (
    set "FOUND_ALL_BUILD_TARGET=true"
) else (
    set "BuildType=%~1"
)
shift
goto :parse_args
:args_done

if "%FOUND_BUILD_TARGET%"=="false" (
    echo No build target selected, building all.
    set "FOUND_ALL_BUILD_TARGET=true"
)

if "%FOUND_ALL_BUILD_TARGET%"=="true" (
    echo Enabling building autoinput tests
    set "BUILD_TESTS=ON"
    echo Enabling building autoinput system tray
    set "BUILD_TRAY=ON"
    echo Enabling building autoinput ui app
    set "BUILD_UI=ON"
)

pushd "%CURRENT_DIR%\.." || exit /b 1
    if "%CLEAN%"=="true" (
        if exist build (
            echo Cleaning build directory...
            rmdir /s /q build || goto :end
        )
    )

    if not exist build (
        echo creating build directory.
        mkdir build || goto :end
    )

    pushd build || goto :end
        echo Running CMake...
cmake -G Ninja ^
-DCMAKE_BUILD_TYPE=%BuildType% ^
-DAUTOINPUT_BUILD_TESTS=%BUILD_TESTS% ^
-DAUTOINPUT_BUILD_TRAY=%BUILD_TRAY% ^
-DAUTOINPUT_BUILD_UI=%BUILD_UI% ^
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
