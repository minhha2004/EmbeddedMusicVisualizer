cmake_minimum_required(VERSION 3.10)
project(MusicVisualizer C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

# Path to FFmpeg build
set(FFMPEG_ROOT "${CMAKE_SOURCE_DIR}/SharedLib/FFmpeg/ffmpeg-build")

# Include directories (FFmpeg headers, LVGL headers, project headers)
include_directories(
    ${FFMPEG_ROOT}/include
    ${CMAKE_SOURCE_DIR}/Graphic
    ${CMAKE_SOURCE_DIR}/Graphic/lvgl
    ${CMAKE_SOURCE_DIR}/Graphic/lvgl/src
    ${CMAKE_SOURCE_DIR}/Graphic/lv_drivers
    ${CMAKE_SOURCE_DIR}/Graphic/lv_drivers/sdl
    ${CMAKE_SOURCE_DIR}/Graphic/music_visualizer_pages
    ${CMAKE_SOURCE_DIR}/MusicProcessor
)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# Link directories for FFmpeg libs
link_directories(${FFMPEG_ROOT}/lib)

# Find FFmpeg libraries (static/shared)
find_library(AVFORMAT_LIB avformat PATHS ${FFMPEG_ROOT}/lib NO_DEFAULT_PATH)
find_library(AVCODEC_LIB avcodec PATHS ${FFMPEG_ROOT}/lib NO_DEFAULT_PATH)
find_library(AVUTIL_LIB avutil PATHS ${FFMPEG_ROOT}/lib NO_DEFAULT_PATH)
find_library(AVDEVICE_LIB avdevice PATHS ${FFMPEG_ROOT}/lib NO_DEFAULT_PATH)
find_library(SWRESAMPLE_LIB swresample PATHS ${FFMPEG_ROOT}/lib NO_DEFAULT_PATH)
find_library(SWSCALE_LIB swscale PATHS ${FFMPEG_ROOT}/lib NO_DEFAULT_PATH)

if(NOT AVFORMAT_LIB)
    message(FATAL_ERROR "libavformat not found in ${FFMPEG_ROOT}/lib")
endif()
if(NOT AVCODEC_LIB)
    message(FATAL_ERROR "libavcodec not found in ${FFMPEG_ROOT}/lib")
endif()
if(NOT AVUTIL_LIB)
    message(FATAL_ERROR "libavutil not found in ${FFMPEG_ROOT}/lib")
endif()

# Collect project sources
file(GLOB_RECURSE GRAPHIC_SOURCES
    "${CMAKE_SOURCE_DIR}/Graphic/*.c"
    "${CMAKE_SOURCE_DIR}/Graphic/*.cpp"
)

# Exclude remote lvgl (if any)
list(FILTER GRAPHIC_SOURCES EXCLUDE REGEX ".*/lvgl\\.remote/.*")
list(FILTER GRAPHIC_SOURCES EXCLUDE REGEX ".*/lv_drivers\\.remote/.*")

# If you want to compile LVGL and lv_drivers sources into the binary, also gather them:
file(GLOB_RECURSE LVGL_SOURCES "${CMAKE_SOURCE_DIR}/Graphic/lvgl/src/*.c")
file(GLOB_RECURSE LV_DRIVERS_SOURCES "${CMAKE_SOURCE_DIR}/Graphic/lv_drivers/*/*.c")

# Music processor sources
file(GLOB_RECURSE MUSICPROCESSOR_SOURCES "${CMAKE_SOURCE_DIR}/MusicProcessor/*.c")
# If you have custom pages
file(GLOB_RECURSE MV_PAGES_SOURCES "${CMAKE_SOURCE_DIR}/Graphic/music_visualizer_pages/*.c")

# Build executable
add_executable(musicvisualizer
    main.cpp
    Graphic/graphic.c
    Graphic/main_page/mainpage.c
    
    # 3. Quản lý chuyển trang (Manager)
    Graphic/music_visualizer_pages/mvpage.c

    # 4. Các trang Visualizer (Logic hiển thị)
    Graphic/music_visualizer_pages/circular.c
    Graphic/music_visualizer_pages/basicmusicvisual.c 

    # 5. Xử lý âm thanh (Audio Processor)
    MusicProcessor/musicprocessor.c

    # 6. --- QUAN TRỌNG: CÁC FILE ẢNH ĐÃ ĐƯỢC CONVERT ---
    Graphic/music_visualizer_pages/circle_bg_png.c
    Graphic/music_visualizer_pages/back_icon_png.c
    Graphic/music_visualizer_pages/arcreactor.c
    Graphic/music_visualizer_pages/particlefountain.c
    Graphic/music_visualizer_pages/peakmeter.c
    ${GRAPHIC_SOURCES}
    ${LVGL_SOURCES}
    ${LV_DRIVERS_SOURCES}
    ${MUSICPROCESSOR_SOURCES}
    ${MV_PAGES_SOURCES}
)

# Link libraries
target_link_libraries(musicvisualizer
    ${AVFORMAT_LIB}
    ${AVCODEC_LIB}
    ${AVUTIL_LIB}
    ${AVDEVICE_LIB}
    ${SWRESAMPLE_LIB}
    ${SWSCALE_LIB}
    pthread
    m
    z
    ${SDL2_LIBRARIES}
)

# Set RPATH so runtime can find shared libs in ffmpeg-build/lib
set_target_properties(musicvisualizer PROPERTIES
    INSTALL_RPATH "${FFMPEG_ROOT}/lib"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# Compiler warnings and optimization
target_compile_options(musicvisualizer PRIVATE -Wall -Wextra -O2)

message(STATUS "FFmpeg root: ${FFMPEG_ROOT}")
message(STATUS "Found SDL2: ${SDL2_LIBRARIES}")
message(STATUS "GRAPHIC_SOURCES count: ${GRAPHIC_SOURCES}")
