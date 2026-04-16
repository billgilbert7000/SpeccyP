add_library(pcm5122 INTERFACE)

target_sources(pcm5122 INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/pcm5122_init.c
        ${CMAKE_CURRENT_LIST_DIR}/pcm5122_init.h
)

target_link_libraries(pcm5122 INTERFACE hardware_i2c)

target_include_directories(pcm5122 INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)
