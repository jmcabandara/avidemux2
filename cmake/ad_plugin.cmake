MACRO(INIT_AUDIO_PLUGIN _lib)
	INCLUDE_DIRECTORIES("${AVIDEMUX_CORECONFIG_DIR}/config")
	INCLUDE_DIRECTORIES("${AVIDEMUX_SOURCE_DIR}/avidemux/ADM_core/include")
	INCLUDE_DIRECTORIES("${AVIDEMUX_SOURCE_DIR}/avidemux/ADM_coreAudio/include")
	INCLUDE_DIRECTORIES("${AVIDEMUX_SOURCE_DIR}/avidemux/ADM_audiocodec")
	INCLUDE_DIRECTORIES("${AVIDEMUX_SOURCE_DIR}/avidemux/ADM_coreUtils/include")
ENDMACRO(INIT_AUDIO_PLUGIN)

MACRO(INSTALL_AUDIODECODER _lib)
	INSTALL(TARGETS ${_lib} DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/ADM_plugins/audioDecoder/")
ENDMACRO(INSTALL_AUDIODECODER)
