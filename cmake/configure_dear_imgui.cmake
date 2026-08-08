#==========================================================
# Configure Raylib
# Project URL: https://github.com/ocornut/imgui.git
# License MIT: https://github.com/ocornut/imgui/blob/v1.92.9/LICENSE.txt
#==========================================================
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

# 1. Fetch Dear ImGui
FetchContent_Declare(
    ImGui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.92.9
)

FetchContent_GetProperties(ImGui)
if (NOT ImGui_POPULATED) # Have we downloaded raylib yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(ImGui)
endif()
FetchContent_GetProperties(ImGui
        SOURCE_DIR ImGui_dir
        POPULATED  ImGui_populated
)
if (ImGui_populated)
    message("ImGui_dir=${ImGui_dir}")
endif()
