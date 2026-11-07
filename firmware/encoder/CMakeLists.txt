cmake_minimum_required(VERSION 3.12.0)

set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/app.overlay")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(encoder)

target_include_directories(app PRIVATE 
    src
    include
    include/hamming74_encoder
    include/huffman_encoder
    include/sht30_driver
    )

target_sources(app PRIVATE
    src/main.c
    include/hamming74_encoder/hamming74_encoder.c
    include/huffman_encoder/huffman_encoder.c
    include/sht30_driver/sht30_driver.c
    )