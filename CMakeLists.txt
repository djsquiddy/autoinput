cmake_minimum_required(VERSION 4.2)
project(autoinput)
include(FetchContent)

set(CMAKE_CXX_STANDARD 23)
set(FETCHCONTENT_QUIET FALSE)

option(ENABLE_KEYBOARD_HOOK "Enable the keyboard listeners" ON)

if (ENABLE_KEYBOARD_HOOK)
    add_compile_definitions(-DAUTOINPUT_HOOK_KEYBOARD_ENABLED=1)
else(ENABLE_KEYBOARD_HOOK)
    add_compile_definitions(-DAUTOINPUT_HOOK_KEYBOARD_ENABLED=0)
endif(ENABLE_KEYBOARD_HOOK)

option(ENABLE_MOUSE_HOOK "Enable the mouse listeners" OFF)
if (ENABLE_MOUSE_HOOK)
    add_compile_definitions(-DAUTOINPUT_HOOK_MOUSE_ENABLED=1)
else(ENABLE_MOUSE_HOOK)
    add_compile_definitions(-DAUTOINPUT_HOOK_MOUSE_ENABLED=0)
endif(ENABLE_MOUSE_HOOK)

option(ENABLE_FAKE_HOOK "Enable only logging the events instead of actually performing them" OFF)
if (ENABLE_FAKE_HOOK)
    add_compile_definitions(-DAUTOINPUT_FAKE_HOOK=1)
else(ENABLE_FAKE_HOOK)
    add_compile_definitions(-DAUTOINPUT_FAKE_HOOK=0)
endif(ENABLE_FAKE_HOOK)


if (NOT PROJECT_SOURCES)
    set(PROJECT_SOURCES)
endif()

set(PROJECT_SOURCES ${PROJECT_SOURCES}
        src/autoinput/autoinput.h
        src/autoinput/arguments.cpp
        src/autoinput/arguments.h
        src/autoinput/types.h
        src/autoinput/types.cpp
        src/autoinput/mouse.h
        src/autoinput/keyboard.h
        src/autoinput/platform.h
        src/autoinput/autoinput.cpp
)
if (WIN32)
    add_compile_definitions(-DNOGDI)
    set(PROJECT_SOURCES ${PROJECT_SOURCES} src/autoinput/win32/autoinput_windows.cpp)
endif(WIN32)

if (NOT PROJECT_INCLUDE)
    set(PROJECT_INCLUDE)
endif()

set(PROJECT_INCLUDE ${PROJECT_INCLUDE} src)

# Declaring our executable
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE src/autoinput/main.cpp ${PROJECT_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_INCLUDE})
