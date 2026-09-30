cmake_minimum_required(VERSION 3.16)
project(job_ranking_engine CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(Threads REQUIRED)

if(MSVC)
    add_compile_options(/O2 /W3)
else()
    add_compile_options(-O2 -Wall -Wextra)
endif()

set(BACKEND_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# --- REST API server (SPEC §4) ---------------------------------------------
add_executable(server main.cpp)
target_include_directories(server PRIVATE ${BACKEND_DIR})
target_link_libraries(server PRIVATE Threads::Threads)
if(WIN32)
    target_link_libraries(server PRIVATE ws2_32)
endif()

# --- CLI benchmark runner (SPEC §9) ----------------------------------------
add_executable(run_benchmarks ${CMAKE_CURRENT_SOURCE_DIR}/../experiments/run_benchmarks.cpp)
target_include_directories(run_benchmarks PRIVATE ${BACKEND_DIR})
target_link_libraries(run_benchmarks PRIVATE Threads::Threads)
