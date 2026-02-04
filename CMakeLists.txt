cmake_minimum_required(VERSION 3.20)

project(audio_streaming_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Common include path
# --------------------------------------------------
set(AUDIO_ENGINE_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# --------------------------------------------------
# Sender executable (UDP audio sender)
# --------------------------------------------------
add_executable(audio_sender
    src/sender_main.cpp
)

target_include_directories(audio_sender PRIVATE
    ${AUDIO_ENGINE_INCLUDE_DIR}
)

target_compile_options(audio_sender PRIVATE
    -Wall -Wextra -Wpedantic
)

# NOTE: sender does NOT need Core Audio

# --------------------------------------------------
# Receiver executable (UDP audio receiver + Core Audio)
# --------------------------------------------------
add_executable(audio_receiver
    src/receiver_main.cpp
)

target_include_directories(audio_receiver PRIVATE
    ${AUDIO_ENGINE_INCLUDE_DIR}
)

target_compile_options(audio_receiver PRIVATE
    -Wall -Wextra -Wpedantic
)

target_link_libraries(audio_receiver
    "-framework AudioToolbox"
    "-framework CoreAudio"
)
