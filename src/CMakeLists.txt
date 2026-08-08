add_subdirectory(autoinput)
add_subdirectory(autoinput_cli)

if(AUTOINPUT_BUILD_TRAY)
    add_subdirectory(autoinput_tray)
endif(AUTOINPUT_BUILD_TRAY)

if(AUTOINPUT_BUILD_UI)
    add_subdirectory(autoinput_ui)
endif(AUTOINPUT_BUILD_UI)
