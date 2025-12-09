cmake_minimum_required(VERSION 3.10)
project(MusicVizSystemC C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# KissFFT paths (relative to systemc_sim/)
set(KISSFFT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../MusicProcessor/kissfft)

add_executable(musicviz_sc_sim
  ${CMAKE_CURRENT_SOURCE_DIR}/sc_main.cpp
  ${KISSFFT_DIR}/kiss_fft.c
  ${KISSFFT_DIR}/kiss_fftr.c
)

target_include_directories(musicviz_sc_sim PRIVATE
  ${KISSFFT_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/../MusicProcessor
)

# SystemC via pkg-config if available
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(SYSTEMC QUIET systemc)
endif()

if(SYSTEMC_FOUND)
  target_include_directories(musicviz_sc_sim PRIVATE ${SYSTEMC_INCLUDE_DIRS})
  target_link_directories(musicviz_sc_sim PRIVATE ${SYSTEMC_LIBRARY_DIRS})
  target_link_libraries(musicviz_sc_sim PRIVATE ${SYSTEMC_LIBRARIES})
else()
  find_path(SYSTEMC_INCLUDE_DIR systemc.h HINTS /usr/include /usr/local/include)
  find_library(SYSTEMC_LIB systemc HINTS /usr/lib /usr/local/lib)

  if(NOT SYSTEMC_INCLUDE_DIR OR NOT SYSTEMC_LIB)
    message(FATAL_ERROR "SystemC not found. Install: sudo apt-get install libsystemc-dev")
  endif()

  target_include_directories(musicviz_sc_sim PRIVATE ${SYSTEMC_INCLUDE_DIR})
  target_link_libraries(musicviz_sc_sim PRIVATE ${SYSTEMC_LIB})
endif()
