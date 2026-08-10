#==========================================================
# Configure Toml++
# Project URL: https://marzer.github.io/tomlplusplus/
# License MIT: https://github.com/marzer/tomlplusplus/blob/v3.4.0/LICENSE
#==========================================================
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.4.0
        SYSTEM  # <--- This suppresses warnings for all targets in this fetch
)
FetchContent_GetProperties(tomlplusplus)
if (NOT tomlplusplus_POPULATED) # Have we downloaded tomlplusplus yet?
    set(FETCHCONTENT_QUIET NO)
    FetchContent_MakeAvailable(tomlplusplus)
endif()
FetchContent_GetProperties(tomlplusplus
        SOURCE_DIR tomlplusplus_dir
        POPULATED  tomlplusplus_populated
)
if (tomlplusplus_populated)
    message("tomlplusplus_dir=${tomlplusplus_dir}")
endif()
