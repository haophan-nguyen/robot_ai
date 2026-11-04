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
