cmake_minimum_required(VERSION 3.10)
project(ParallelPixels C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_BUILD_TYPE Debug)

# Find pthreads (likely needed for threading within the single process)
find_package(Threads REQUIRED)
find_library(MATH_LIBRARY m)

# Main Executable
add_executable(ppxl
    main.c

    pipeline/chunking/src/directory_monitor.c
    pipeline/chunking/src/file_tracker.c
    pipeline/chunking/src/image_chunker.c
    pipeline/chunking/src/image_queue.c 
    
    pipeline/filter/src/chunk_threader.c
    pipeline/filter/src/filter.c
    
    shared/Object.c
    shared/darray.c
    shared/dlist.c
    shared/dict.c
    shared/image.c
    shared/thread_pool.c

    pipeline/reconstruction/image_unchunk.c
    pipeline/reconstruction/reconstruction.c
)

add_library(xxhash STATIC ${CMAKE_CURRENT_SOURCE_DIR}/vendors/xxHash/xxhash.c)

target_include_directories(ppxl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/chunking/include
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/filter/include
    ${CMAKE_CURRENT_SOURCE_DIR}/pipeline/reconstruction

    ${CMAKE_CURRENT_SOURCE_DIR}/vendors/xxHash
)

target_link_libraries(ppxl PRIVATE
    xxhash
    Threads::Threads
    ${MATH_LIBRARY}
)

# Optional: For extra compile options (warnings, debugging)
# target_compile_options(main PRIVATE -Wall -Wextra -pedantic)
