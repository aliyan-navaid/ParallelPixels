cmake_minimum_required(VERSION 3.10)
project(ParallelPixels C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

find_package(Threads REQUIRED)
find_library(MATH_LIBRARY m)

add_executable(main
    main.c

    pipeline/chunking/src/directory_monitor.c
    pipeline/chunking/src/file_tracker.c
    pipeline/chunking/src/image_chunker.c
    pipeline/filter/src/chunk_threader.c
    pipeline/filter/src/filter.c
    pipeline/chunking/src/image_queue.c 

    shared/image.c
)

target_include_directories(main PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/shared                   # Shared types (image.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/chunking/include # Chunking headers + uthash
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/filter/include # Filtering headers
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/reconstruction # Reconstruction headers
)

target_link_libraries(main PRIVATE
    Threads::Threads
    ${MATH_LIBRARY}
)