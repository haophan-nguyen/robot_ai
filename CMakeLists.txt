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
option(AI_ROBOT_ENABLE_AUDIO_CAPTURE "Build the F002 WAV capture CLI" OFF)

if(AI_ROBOT_ENABLE_AUDIO_PLAYBACK AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Audio playback requires a Linux target with ALSA.")
endif()
if(AI_ROBOT_ENABLE_AUDIO_CAPTURE AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Audio capture requires a Linux target with ALSA.")
endif()

if(AI_ROBOT_ENABLE_AUDIO_PLAYBACK OR AI_ROBOT_ENABLE_AUDIO_CAPTURE)
    find_package(ALSA REQUIRED)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SNDFILE REQUIRED IMPORTED_TARGET sndfile)
    if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
    endif()
endif()

if(AI_ROBOT_ENABLE_AUDIO_PLAYBACK)
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

if(AI_ROBOT_ENABLE_AUDIO_CAPTURE)
    add_executable(ai_robot_record_wav
        software/src/audio/record_wav_main.cpp
        software/src/audio/wav_capture.cpp
    )
    set_target_properties(ai_robot_record_wav PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
    )
    target_link_libraries(ai_robot_record_wav PRIVATE ALSA::ALSA PkgConfig::SNDFILE)

    if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
        add_test(NAME audio_capture
            COMMAND Python3::Interpreter
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/audio_capture_test.py"
                "$<TARGET_FILE:ai_robot_record_wav>"
        )
        set_tests_properties(audio_capture PROPERTIES TIMEOUT 30)
    endif()
endif()
