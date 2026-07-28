#==========================================================
# Configure GSL (Guidelines Support Library)
# Project URL: https://github.com/microsoft/GSL
# License MIT: https://github.com/microsoft/GSL/blob/v4.2.2/LICENSE
#==========================================================
include(FetchContent)
FetchContent_Declare(
        GSL
        GIT_REPOSITORY "https://github.com/microsoft/GSL.git"
        GIT_TAG "v4.2.2"
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(GSL)
