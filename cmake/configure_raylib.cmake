#==========================================================
# Configure Raylib
# Project URL: https://github.com/raysan5/raylib
# License zlib/libpng: https://github.com/raysan5/raylib/blob/master/LICENSE
#==========================================================
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE) # don't build the supplied examples
set(BUILD_GAMES    OFF CACHE BOOL "" FORCE) # don't build the supplied example games
set(RAYLIB_VERSION 6.0)

FetchContent_Declare(
    raylib
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    SYSTEM  # <--- This suppresses warnings for all targets in this fetch
)
FetchContent_GetProperties(raylib)
if (NOT raylib_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(raylib)
endif()
FetchContent_GetProperties(raylib
    SOURCE_DIR raylib_dir
    POPULATED  raylib_populated
)
if (raylib_populated)
    message("raylib_dir=${raylib_dir}")
endif()

# Disable specific warning for raylib target
target_compile_options(raylib PRIVATE -Wno-tautological-compare)

set_target_properties(raylib PROPERTIES UNITY_BUILD OFF)
if (TARGET glfw)
    set_target_properties(glfw PROPERTIES UNITY_BUILD OFF)
endif()
