#==========================================================
# Configure Googletest
#==========================================================
include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        52eb8108c5bdec04579160ae17225d66034bd723 # release-1.17.0
  FIND_PACKAGE_ARGS NAMES GTest
  SYSTEM  # <--- This suppresses warnings for all targets in this fetch
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)
# EXPLICITLY disable the specific warning for the gtest targets
# This is required because the library is compiled, not just included
target_compile_options(gtest PRIVATE -Wno-character-conversion)
target_compile_options(gtest_main PRIVATE -Wno-character-conversion)
target_compile_options(gmock PRIVATE -Wno-character-conversion)
target_compile_options(gmock_main PRIVATE -Wno-character-conversion)

# Disable all warnings for the gtest and gmock targets
# If you are treating warnings as errors globally, ensure it's off for these targets
set_target_properties(gtest gtest_main gmock gmock_main PROPERTIES
        CXX_CLANG_WARNING_AS_ERROR OFF
        CXX_GCC_WARNING_AS_ERROR OFF
        COMPILE_WARNING_AS_ERROR OFF
)