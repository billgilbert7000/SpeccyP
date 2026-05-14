add_library(audio_i2s INTERFACE)

# Когда включён HDMI_HSTX, i2s_init/i2s_out/i2s_deinit приходят из
# drivers/hdmi_hstx/HDMI_hstx_audio.c, и тащить сюда исходник PIO/DMA
# не нужно — иначе линкер словит multiple-definition. Заголовок
# audio_i2s.h всё равно остаётся публичным API, плюс мы продолжаем
# генерить audio_i2s.pio.h: он подтягивается через include в
# audio_i2s.h, и pico_generate_pio_header() нужно куда-то прицепить
# — пусть будет наш audio_i2s INTERFACE.
if(HDMI_HSTX)
    target_sources(audio_i2s INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/audio_i2s.h
    )
else()
    target_sources(audio_i2s INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/audio_i2s.c
        ${CMAKE_CURRENT_LIST_DIR}/audio_i2s.h
    )
endif()

target_link_libraries(audio_i2s INTERFACE hardware_pio hardware_clocks)

target_include_directories(audio_i2s INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)

pico_generate_pio_header(audio_i2s
  ${CMAKE_CURRENT_LIST_DIR}/audio_i2s.pio
)
