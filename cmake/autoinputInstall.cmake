include(GNUInstallDirs)

function(autoinput_set_global_if_not_defined var_name default_value doc_string)
    if(NOT DEFINED ${var_name})
        set(${var_name} "${default_value}" CACHE PATH "${doc_string}")
    endif()
endfunction()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_SOURCE_DIR}/output/" CACHE PATH "Installation prefix" FORCE)
endif()

autoinput_set_global_if_not_defined(
        CMAKE_RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/bin"
        "Where to place the executables."
)

autoinput_set_global_if_not_defined(
        CMAKE_LIBRARY_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/lib"
        "Where to place compiled dynamic libraries."
)

autoinput_set_global_if_not_defined(
        CMAKE_ARCHIVE_OUTPUT_DIRECTORY
        "${CMAKE_BINARY_DIR}/lib"
        "Where to place compiled static libraries."
)
