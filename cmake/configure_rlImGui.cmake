#==========================================================
# Configure rlImGui
# Project URL: https://github.com/raylib-extras/rlImGui
# License zlib: https://github.com/raylib-extras/rlImGui/blob/Raylib_6_0/LICENSE
#==========================================================
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# Replace all occurrences of '.' with '_'
string(REPLACE "." "_" URL_RAYLIB_VERSION "${RAYLIB_VERSION}")

# 2. Fetch rlImGui
FetchContent_Declare(
    rlImGui
    GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
    GIT_TAG "Raylib_${URL_RAYLIB_VERSION}"
)
FetchContent_MakeAvailable(rlImGui)
