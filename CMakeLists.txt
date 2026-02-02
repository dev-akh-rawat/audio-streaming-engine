cmake_minimum_required(VERSION 3.20)

project(audio_streaming_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(audio_streaming_engine
    src/main.cpp
)

target_include_directories(audio_streaming_engine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_options(audio_streaming_engine PRIVATE
    -Wall -Wextra -Wpedantic
)

target_link_libraries(audio_streaming_engine
    "-framework AudioToolbox"
    "-framework CoreAudio"
)

