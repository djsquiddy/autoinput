#==========================================================
# Configure Raylib
# Project URL: https://github.com/microsoft/GSL
# License MIT: https://github.com/microsoft/GSL/blob/v4.2.2/LICENS
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
