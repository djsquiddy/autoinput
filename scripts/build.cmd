@echo off
setlocal
set "CURRENT_DIR=%~dp0"

pushd "%CURRENT_DIR%\.."

    if not exist build (
        echo creating build directory.
        mkdir build
    )

    pushd build
        echo Running CMake...
        cmake -G Ninja .. || goto :end
        echo Finished running CMake.
        echo Rebuilding...
        ninja || goto :end
        echo Finished rebuilding.
        echo Running tests...
        .\autoinput_tests.exe || goto :end
        echo Finished running tests.
    popd
popd

:end
    endlocal
    goto :EOF
