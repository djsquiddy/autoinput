
# Sets Static Runtime: /MTd for Debug, /MT for Release
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

function(configure_autoinput_platform target_name)
    if(UNIX AND NOT APPLE)
        find_package(X11)
        find_library(XTST_LIBRARY Xtst)

        if(X11_FOUND AND XTST_LIBRARY)
            target_link_libraries(${target_name}
                    PUBLIC
                    ${X11_LIBRARIES}
                    ${XTST_LIBRARY}
            )

            target_compile_definitions(${target_name} PUBLIC AUTOINPUT_WITH_X11=1)
        else()
            target_compile_definitions(${target_name} PUBLIC AUTOINPUT_WITH_X11=0)
            message(WARNING "X11 or XTest not found. X11 backend will be disabled.")
        endif()
    endif()
endfunction()
