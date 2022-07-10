cmake_minimum_required(VERSION 3.20)
project(RAID6
        VERSION 0.1.0
        LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
  message(FATAL_ERROR "In-source builds are not allowed")
endif()

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(src bin)
