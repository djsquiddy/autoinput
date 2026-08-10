function(configure_autoinput_target target_name)
    target_compile_definitions(${target_name} PUBLIC AUTOINPUT_VERSION="${PROJECT_VERSION}")

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

    if(AUTOINPUT_BUILD_TESTS)
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
        if(MINGW)
            # Contains a compatibility fixes for MinGW static linking issues.
            target_sources(${target_name} PRIVATE ${CMAKE_SOURCE_DIR}/src/autoinput/platform/mingw/mingw_compat.cpp)
            target_link_options(${target_name} PRIVATE -static)
        endif()
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

    get_target_property(TARGET_TYPE ${target_name} TYPE)
    if(TARGET_TYPE STREQUAL "EXECUTABLE")
        add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/resources"
                "$<TARGET_FILE_DIR:${target_name}>/resources"
                COMMENT "Copying resources to output directory for ${target_name}"
        )

        add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_SOURCE_DIR}/configs"
                "$<TARGET_FILE_DIR:${target_name}>/configs"
                COMMENT "Copying configs to output directory for ${target_name}"
        )
    endif()
endfunction()
