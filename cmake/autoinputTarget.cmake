function(autoinput_group_sources_by_directory base_dir)
    foreach(source_file ${ARGN})
        get_filename_component(file_ext "${source_file}" EXT)
        if(file_ext MATCHES "\\.(cpp|cxx|c|cc)$")
            get_filename_component(file_dir "${source_file}" DIRECTORY)
            file(RELATIVE_PATH rel_dir "${base_dir}" "${file_dir}")
            if(rel_dir AND NOT rel_dir STREQUAL "." AND NOT rel_dir MATCHES "^\\.\\.")
                string(REPLACE "/" "_" group_name "${rel_dir}")
                string(REPLACE "\\" "_" group_name "${group_name}")
                set_source_files_properties("${source_file}" PROPERTIES UNITY_GROUP "${group_name}")
            endif()
        endif()
    endforeach()
endfunction()

function(configure_autoinput_target target_name)
    set_target_properties(${target_name} PROPERTIES
            UNITY_BUILD_MODE GROUP
            CXX_SCAN_FOR_MODULES OFF
    )

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

#    target_precompile_headers(${target_name}
#            PRIVATE
#            ${CMAKE_SOURCE_DIR}/src/pch.h
#    )

    install(TARGETS ${target_name}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )

    get_target_property(TARGET_TYPE ${target_name} TYPE)
    if(TARGET_TYPE STREQUAL "EXECUTABLE")
        if(NOT TARGET autoinput_resources)
            add_custom_target(autoinput_resources
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_SOURCE_DIR}/resources"
                    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources"
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    "${CMAKE_SOURCE_DIR}/configs"
                    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/configs"
                    COMMENT "Copying shared resources and configs to output directory"
            )
        endif()
        add_dependencies(${target_name} autoinput_resources)
        if(WIN32)
            target_sources(${target_name} PRIVATE "${APP_ICON_OUTPUT_RC}")
            add_dependencies(${target_name} generate_app_icon)
        endif()
    endif()
endfunction()
