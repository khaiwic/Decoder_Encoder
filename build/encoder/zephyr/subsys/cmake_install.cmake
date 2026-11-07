# Install script for directory: C:/zephyrproject/zephyr/subsys

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/Zephyr-Kernel")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "TRUE")
endif()

# Set path to fallback-tool for dependency-resolution.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "C:/zephyr-sdk-1.0.1/gnu/xtensa-espressif_esp32s3_zephyr-elf/bin/xtensa-espressif_esp32s3_zephyr-elf-objdump.exe")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/authentication/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/canbus/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/debug/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/fs/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/gnss/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/instrumentation/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/ipc/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/kvss/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/logging/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/mem_mgmt/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/mgmt/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/pm/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/pmci/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/portability/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/random/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/sd/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/stats/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/storage/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/testsuite/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/tracing/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/usb/cmake_install.cmake")
  include("C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/crc/cmake_install.cmake")

endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
if(CMAKE_INSTALL_LOCAL_ONLY)
  file(WRITE "C:/Users/ADMIN/Documents/Decoder_and_Encoder/build/encoder/zephyr/subsys/install_local_manifest.txt"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
endif()
