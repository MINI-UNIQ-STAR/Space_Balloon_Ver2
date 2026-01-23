# -----------------------------------------------------------------------------
# Host Test Build - Simplified Algorithm Testing
# -----------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.20)
project(host_test_runner C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Coverage flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 --coverage -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")

# Only algorithm sources (no HAL dependencies)
set(HOST_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/test_algorithms.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Src/pid.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Src/kalman.c
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Inc
)

# Create executable
add_executable(${PROJECT_NAME} ${HOST_TEST_SOURCES})

# Link math library
target_link_libraries(${PROJECT_NAME} m)

message(STATUS "Simplified host test: PID + Kalman only")
message(STATUS "Run: ./${PROJECT_NAME}")
message(STATUS "Coverage: gcov -b pid.c kalman.c")
