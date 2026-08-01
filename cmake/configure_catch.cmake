include(FetchContent)
set(FETCHCONTENT_QUIET FALSE)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        605a34765aa5d5ecbf476b4598a862ada971b0cc # v3.0.1
  FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(Catch2)
