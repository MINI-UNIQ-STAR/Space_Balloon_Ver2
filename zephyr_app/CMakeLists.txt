cmake_minimum_required(VERSION 3.20.0)

# Board root for custom board
set(BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/boards)

# Zephyr 찾기
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(space_balloon_ver2 LANGUAGES C)

# 포팅된 소스 파일
set(PORTED_SOURCES
    src/main.c
    src/fdir_zephyr.c
    src/sensors_zephyr.c
    src/telemetry_zephyr.c
    src/kalman.c
    src/pid.c
    src/actuators_zephyr.c
)

# 직접 구현한 센서 드라이버
set(CUSTOM_SENSOR_SOURCES
    src/sensors/mlx90393.c
    src/sensors/gdk101.c
    src/sensors/cm1107n.c
    src/sensors/sen0321.c
)

# Zephyr 앱으로 빌드
target_sources(app PRIVATE ${PORTED_SOURCES} ${CUSTOM_SENSOR_SOURCES})

# Include 경로
target_include_directories(app PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# C11 표준
target_compile_options(app PRIVATE -std=c11)

# 디버그 심볼
target_compile_options(app PRIVATE -g)
