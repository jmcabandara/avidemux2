INCLUDE(ae_plugin)
SET(ADM_ae_opus_SRCS audioencoder_opus.cpp)
ADD_AUDIO_ENCODER(ADM_ae_opus ${ADM_ae_opus_SRCS})
TARGET_LINK_LIBRARIES(ADM_ae_opus ${OPUS_ENC_LIBRARY_DIR})
ADD_TARGET_CFLAGS(ADM_ae_opus "-I${OPUS_ENC_INCLUDE_DIR}")
INIT_AUDIO_ENCODER(ADM_ae_opus)
INSTALL_AUDIOENCODER(ADM_ae_opus)
