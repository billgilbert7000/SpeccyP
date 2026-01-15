add_library(audio_i2s INTERFACE)

target_sources(audio_i2s INTERFACE
		${CMAKE_CURRENT_LIST_DIR}/audio_i2s.c
		${CMAKE_CURRENT_LIST_DIR}/audio_i2s.h
)

target_link_libraries(audio_i2s INTERFACE hardware_pio hardware_clocks)

target_include_directories(audio_i2s INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)

pico_generate_pio_header(audio_i2s
  ${CMAKE_CURRENT_LIST_DIR}/audio_i2s.pio
)
