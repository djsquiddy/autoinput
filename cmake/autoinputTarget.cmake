function(configure_autoinput_target target_name)
    if(ENABLE_KEYBOARD_HOOK)
        target_compile_definitions(${target_name} PUBLIC AUTOINPUT_HOOK_KEYBOARD_ENABLED=1)
    else()
        target_compile_definitions(${target_name} PUBLIC AUTOINPUT_HOOK_KEYBOARD_ENABLED=0)
    endif()

    if(ENABLE_MOUSE_HOOK)
        target_compile_definitions(${target_name} PUBLIC AUTOINPUT_HOOK_MOUSE_ENABLED=1)
    else()
        target_compile_definitions(${target_name} PUBLIC AUTOINPUT_HOOK_MOUSE_ENABLED=0)
    endif()

    if(ENABLE_FAKE_HOOK)
        target_compile_definitions(${target_name} PUBLIC AUTOINPUT_FAKE_HOOK=1)
    else()
        target_compile_definitions(${target_name} PUBLIC AUTOINPUT_FAKE_HOOK=0)
    endif()

    if(target_name STREQUAL "${PROJECT_NAME}_tests")
        target_compile_definitions(${target_name} PRIVATE AUTOINPUT_TESTING=1)
    endif()

    target_compile_definitions(${target_name}
            PUBLIC
            TOML_HEADER_ONLY=0
            $<$<CONFIG:Debug>:AUTOINPUT_DEBUG>
            $<$<CONFIG:Release>:AUTOINPUT_RELEASE>
    )

    if(WIN32)
        target_compile_definitions(${target_name}
                PUBLIC
                NOGDI
                NOMINMAX
                _CRT_SECURE_NO_WARNINGS
        )
    endif()

    target_precompile_headers(${target_name}
            PRIVATE
            ${CMAKE_SOURCE_DIR}/src/pch.h
    )

    install(TARGETS ${target_name}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endfunction()
