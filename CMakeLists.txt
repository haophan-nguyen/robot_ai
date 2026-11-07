cmake_minimum_required(VERSION 3.16)

project(ai_robot LANGUAGES CXX)

add_executable(ai_robot software/src/main.cpp)
set_target_properties(ai_robot PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

include(CTest)
if(BUILD_TESTING)
    add_test(NAME ai_robot_smoke COMMAND ai_robot)
    set_tests_properties(ai_robot_smoke PROPERTIES TIMEOUT 5)
endif()

option(AI_ROBOT_ENABLE_AUDIO_PLAYBACK "Build the F001 WAV playback CLI" OFF)
if(AI_ROBOT_ENABLE_AUDIO_PLAYBACK)
    if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        message(FATAL_ERROR "Audio playback requires a Linux target with ALSA.")
    endif()

    find_package(ALSA REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SNDFILE REQUIRED IMPORTED_TARGET sndfile)

    add_executable(ai_robot_play_wav
        software/src/audio/play_wav_main.cpp
        software/src/audio/wav_playback.cpp
    )
    set_target_properties(ai_robot_play_wav PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
    target_link_libraries(ai_robot_play_wav PRIVATE ALSA::ALSA PkgConfig::SNDFILE)

    if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
        foreach(mode IN ITEMS valid invalid)
            add_test(NAME audio_playback_${mode}
                COMMAND Python3::Interpreter
                    "${CMAKE_CURRENT_SOURCE_DIR}/tests/audio_playback_test.py"
                    "${mode}" "$<TARGET_FILE:ai_robot_play_wav>"
            )
            set_tests_properties(audio_playback_${mode} PROPERTIES TIMEOUT 30)
        endforeach()
    endif()
endif()
