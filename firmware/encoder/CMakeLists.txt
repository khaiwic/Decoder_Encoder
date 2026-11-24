cmake_minimum_required(VERSION 3.12.0)

set(DTC_OVERLAY_FILE "${CMAKE_CURRENT_SOURCE_DIR}/app.overlay")

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(encoder)

# Gộp tạo file .a và đánh chỉ mục symbol trong 1 bước (ar qcs) để tránh lỗi ranlib.exe bị khóa file trên Windows
set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_COMMAND> -E true")
set(CMAKE_ASM_ARCHIVE_CREATE "<CMAKE_AR> qcs <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_ASM_ARCHIVE_FINISH "<CMAKE_COMMAND> -E true")

target_include_directories(app PRIVATE 
    src
    include
    include/hamming74_encoder
    include/huffman_encoder
    include/logger
    include/sht30_driver
    include/control_pannel
    include/Tx
    )

target_sources(app PRIVATE
    src/main.c
    src/factory.c
    include/hamming74_encoder/hamming74_encoder.c
    include/huffman_encoder/huffman_encoder.c
    include/logger/logger.c
    include/sht30_driver/sht30_driver.c
    include/control_pannel/control_pannel.c
    include/Tx/tx.c
    )
