# project (drumkv1)

set (CMAKE_INCLUDE_CURRENT_DIR ON)

set (CMAKE_AUTOUIC ON)
set (CMAKE_AUTOMOC ON)
set (CMAKE_AUTORCC ON)

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
  file (REMOVE ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
endif ()
configure_file (config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set (HEADERS
  drumkv1.h
  drumkv1_config.h
  drumkv1_filter.h
  drumkv1_formant.h
  drumkv1_resampler.h
  drumkv1_sample.h
  drumkv1_wave.h
  drumkv1_ramp.h
  drumkv1_list.h
  drumkv1_fx.h
  drumkv1_reverb.h
  drumkv1_param.h
  drumkv1_sched.h
  drumkv1_tuning.h
  drumkv1_programs.h
  drumkv1_controls.h
)

set (SOURCES
  drumkv1.cpp
  drumkv1_config.cpp
  drumkv1_formant.cpp
  drumkv1_resampler.cpp
  drumkv1_sample.cpp
  drumkv1_wave.cpp
  drumkv1_param.cpp
  drumkv1_sched.cpp
  drumkv1_tuning.cpp
  drumkv1_programs.cpp
  drumkv1_controls.cpp
)


set (HEADERS_UI
  drumkv1_ui.h
  drumkv1widget.h
  drumkv1widget_env.h
  drumkv1widget_filt.h
  drumkv1widget_sample.h
  drumkv1widget_wave.h
  drumkv1widget_param.h
  drumkv1widget_keybd.h
  drumkv1widget_preset.h
  drumkv1widget_status.h
  drumkv1widget_spinbox.h
  drumkv1widget_elements.h
  drumkv1widget_programs.h
  drumkv1widget_controls.h
  drumkv1widget_control.h
  drumkv1widget_palette.h
  drumkv1widget_config.h
)

set (SOURCES_UI
  drumkv1_ui.cpp
  drumkv1widget.cpp
  drumkv1widget_env.cpp
  drumkv1widget_filt.cpp
  drumkv1widget_sample.cpp
  drumkv1widget_wave.cpp
  drumkv1widget_param.cpp
  drumkv1widget_keybd.cpp
  drumkv1widget_preset.cpp
  drumkv1widget_status.cpp
  drumkv1widget_spinbox.cpp
  drumkv1widget_elements.cpp
  drumkv1widget_programs.cpp
  drumkv1widget_controls.cpp
  drumkv1widget_control.cpp
  drumkv1widget_palette.cpp
  drumkv1widget_config.cpp
)

set (FORMS_UI
  drumkv1widget.ui
  drumkv1widget_control.ui
  drumkv1widget_palette.ui
  drumkv1widget_config.ui
)

set (RESOURCES_UI
  drumkv1.qrc
)


set (HEADERS_LV2
  drumkv1_lv2.h
  drumkv1_lv2ui.h
  drumkv1widget_lv2.h
)

set (SOURCES_LV2
  drumkv1_lv2.cpp
  drumkv1_lv2ui.cpp
  drumkv1widget_lv2.cpp
)


set (HEADERS_JACK
  drumkv1_nsm.h
  drumkv1_jack.h
  drumkv1widget_jack.h
)

set (SOURCES_JACK
  drumkv1_nsm.cpp
  drumkv1_jack.cpp
  drumkv1widget_jack.cpp
)


add_library (${PROJECT_NAME} STATIC
  ${HEADERS}
  ${SOURCES}
)

add_library (${PROJECT_NAME}_ui STATIC
  ${HEADERS_UI}
  ${SOURCES_UI}
  ${FORMS_UI}
  ${RESOURCES_UI}
)


if (CONFIG_LV2)
  add_library (${PROJECT_NAME}_lv2 MODULE
    ${HEADERS_LV2}
    ${SOURCES_LV2}
  )
endif ()

if (CONFIG_JACK)
  add_executable (${PROJECT_NAME}_jack
    ${HEADERS_JACK}
    ${SOURCES_JACK}
  )
endif ()

set_target_properties (${PROJECT_NAME}    PROPERTIES CXX_STANDARD 17)
set_target_properties (${PROJECT_NAME}_ui PROPERTIES CXX_STANDARD 17)

if (QT_VERSION VERSION_GREATER_EQUAL 6.0.0)
  target_compile_options (${PROJECT_NAME}    PRIVATE -fPIC)
  target_compile_options (${PROJECT_NAME}_ui PRIVATE -fPIC)
endif ()

# Add some debugger flags.
if (CONFIG_DEBUG AND UNIX AND NOT APPLE)
  set (CONFIG_DEBUG_OPTIONS -g -fsanitize=address -fno-omit-frame-pointer)
  target_compile_options (${PROJECT_NAME} PRIVATE ${CONFIG_DEBUG_OPTIONS})
  target_compile_options (${PROJECT_NAME}_ui PRIVATE ${CONFIG_DEBUG_OPTIONS})
  if (CONFIG_LV2)
    target_compile_options (${PROJECT_NAME}_lv2 PRIVATE ${CONFIG_DEBUG_OPTIONS})
    target_link_options (${PROJECT_NAME}_lv2 PRIVATE ${CONFIG_DEBUG_OPTIONS})
  endif ()
  if (CONFIG_JACK)
    target_compile_options (${PROJECT_NAME}_jack PRIVATE ${CONFIG_DEBUG_OPTIONS})
    target_link_options (${PROJECT_NAME}_jack PRIVATE ${CONFIG_DEBUG_OPTIONS})
  endif ()
endif ()

target_link_libraries (${PROJECT_NAME}    PUBLIC Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Xml)
target_link_libraries (${PROJECT_NAME}_ui PUBLIC Qt${QT_VERSION_MAJOR}::Widgets ${PROJECT_NAME})

if (CONFIG_SNDFILE)
  target_link_libraries (${PROJECT_NAME} PRIVATE PkgConfig::SNDFILE)
endif ()

if (CONFIG_LV2)
  set_target_properties (${PROJECT_NAME}_lv2 PROPERTIES CXX_STANDARD 17)
  target_link_libraries (${PROJECT_NAME}_lv2 PRIVATE ${PROJECT_NAME}_ui)
  target_include_directories (${PROJECT_NAME}_lv2 PRIVATE ${LV2_INCLUDE_DIRS})
  target_link_directories (${PROJECT_NAME}_lv2 PRIVATE ${LV2_LIBRARY_DIRS})
  target_link_libraries (${PROJECT_NAME}_lv2 PRIVATE ${LV2_LIBRARIES})
  if (UNIX AND NOT APPLE)
    if (NOT CONFIG_DEBUG)
      add_custom_command(TARGET ${PROJECT_NAME}_lv2  POST_BUILD
        COMMAND strip lib${PROJECT_NAME}_lv2.so)
    endif ()
    if (CONFIG_PREFIX MATCHES $ENV{HOME})
      set (CONFIG_LV2DIR ${CONFIG_PREFIX}/.lv2)
    else ()
      set (CONFIG_LV2DIR ${CONFIG_LIBDIR}/lv2)
    endif ()
    install (FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}_lv2.so
      RENAME ${PROJECT_NAME}.so
      DESTINATION ${CONFIG_LV2DIR}/${PROJECT_NAME}.lv2)
    install (FILES ${PROJECT_NAME}.lv2/manifest.ttl
      ${PROJECT_NAME}.lv2/${PROJECT_NAME}.ttl
      ${PROJECT_NAME}.lv2/${PROJECT_NAME}_ui.ttl
      DESTINATION ${CONFIG_LV2DIR}/${PROJECT_NAME}.lv2)
  endif ()
  if (WIN32)
    target_link_options (${PROJECT_NAME}_lv2 PRIVATE -static-libgcc -static-libstdc++)
    if (NOT CONFIG_DEBUG)
      add_custom_command(TARGET ${PROJECT_NAME}_lv2  POST_BUILD
        COMMAND strip lib${PROJECT_NAME}_lv2.dll)
    endif ()
    set (CONFIG_LV2DIR ${CONFIG_WINDOWS_LV2_PATH})
    install (FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}_lv2.dll
      RENAME ${PROJECT_NAME}.dll
      DESTINATION ${CONFIG_LV2DIR}/${PROJECT_NAME}.lv2)
    install (FILES ${PROJECT_NAME}.lv2/manifest-win32.ttl
      RENAME manifest.ttl
      DESTINATION ${CONFIG_LV2DIR}/${PROJECT_NAME}.lv2)
    install (FILES ${PROJECT_NAME}.lv2/${PROJECT_NAME}.ttl
      DESTINATION ${CONFIG_LV2DIR}/${PROJECT_NAME}.lv2)
    install (FILES ${PROJECT_NAME}.lv2/${PROJECT_NAME}_ui-win32.ttl
      RENAME ${PROJECT_NAME}_ui.ttl
      DESTINATION ${CONFIG_LV2DIR}/${PROJECT_NAME}.lv2)
  endif ()
endif ()

if (CONFIG_JACK)
  set_target_properties (${PROJECT_NAME}_jack PROPERTIES CXX_STANDARD 17)
  target_link_libraries (${PROJECT_NAME}_jack PRIVATE ${PROJECT_NAME}_ui)
  target_link_libraries (${PROJECT_NAME}_jack PRIVATE PkgConfig::JACK)
  if (CONFIG_ALSA_MIDI)
    target_link_libraries (${PROJECT_NAME}_jack PRIVATE PkgConfig::ALSA)
  endif ()
  if (CONFIG_LIBLO)
    target_link_libraries (${PROJECT_NAME}_jack PRIVATE PkgConfig::LIBLO)
  endif ()
  if (UNIX AND NOT APPLE)
    if (NOT CONFIG_DEBUG)
      add_custom_command(TARGET ${PROJECT_NAME}_jack POST_BUILD
        COMMAND strip ${PROJECT_NAME}_jack)
    endif ()
    install (TARGETS ${PROJECT_NAME}_jack RUNTIME
      DESTINATION ${CMAKE_INSTALL_BINDIR})
    install (FILES images/${PROJECT_NAME}.png
      RENAME org.rncbc.${PROJECT_NAME}.png
      DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/apps)
    install (FILES images/${PROJECT_NAME}.svg
      RENAME org.rncbc.${PROJECT_NAME}.svg
      DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
    install (FILES appdata/org.rncbc.${PROJECT_NAME}.desktop
      DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
    install (FILES appdata/org.rncbc.${PROJECT_NAME}.metainfo.xml
      DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
    install (FILES mimetypes/org.rncbc.${PROJECT_NAME}.xml
      DESTINATION ${CMAKE_INSTALL_DATADIR}/mime/packages)
    install (FILES mimetypes/org.rncbc.${PROJECT_NAME}.application-x-${PROJECT_NAME}-preset.png
      DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/32x32/mimetypes)
    install (FILES mimetypes/org.rncbc.${PROJECT_NAME}.application-x-${PROJECT_NAME}-preset.svg
      DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/mimetypes)
  endif ()
endif ()
