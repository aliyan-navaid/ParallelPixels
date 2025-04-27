# ppxl - Parallel Pixels (Concurrent Image Processing Pipeline)

## Description

`ppxl` is a command-line application designed to process images concurrently using a multi-stage pipeline. It monitors an input directory for new images, breaks them into chunks, applies specified image effects using multiple threads, reconstructs the processed images, and saves them to an output directory. The application displays real-time processing statistics and supports graceful shutdown.

## Features

*   **Directory Monitoring:** Continuously watches a specified input directory for new image files.
*   **Multi-threaded Processing:** Utilizes multiple threads for image chunking and filtering stages to leverage multi-core processors.
*   **Pipeline Architecture:** Employs thread-safe queues to pass data between processing stages (Naming -> Chunking -> Filtering -> Reconstruction).
*   **Configurable Effects:** Allows specifying image effects to be applied via command-line arguments.
*   **Real-time Statistics:** Displays the number of images read, written, and discarded during processing.
*   **Graceful Shutdown:** Handles `SIGINT` and `SIGTERM` signals (e.g., via Ctrl+C or user input 'e') to shut down worker threads cleanly.

## Dependencies

*   A C compiler supporting C11 (for `stdatomic.h`)
*   POSIX Threads (`pthread`) library
*   Standard C libraries
*   (Potentially an image loading/saving library like `libpng`, `libjpeg`, or a custom one if `image.h` relies on it - *Add specific library if known*)
*   CMake (for building)

## Building

1.  **Clone the repository (if applicable):**
    ```bash
    git clone <repository_url>
    cd OS # Or your project root
    ```
2.  **Create a build directory:**
    ```bash
    mkdir build
    cd build
    ```
3.  **Run CMake:**
    ```bash
    cmake ..
    ```
    *   To build in Debug mode (includes `PRINTF` statements): `cmake .. -DCMAKE_BUILD_TYPE=Debug`
    *   To build in Release mode (optimized, `PRINTF` disabled): `cmake .. -DCMAKE_BUILD_TYPE=Release`
4.  **Compile:**
    ```bash
    make
    ```
    The executable `ppxl` will be created in the `build` directory.

## Usage

Run the executable from the `build` directory (or after installing it to your PATH):

```bash
./ppxl <input_directory> -e <effects> [-o <output_directory>]
```

**Arguments:**

*   `<input_directory>`: (Required) Path to the directory containing the images to process.
*   `-e <effects>`: (Required) Specifies the image effects to apply (e.g., `"greyscale"`). The exact format depends on the implementation in `chunk_threader.c` / `process_chunk`.
*   `-o <output_directory>`: (Optional) Path to the directory where processed images will be saved. Defaults to `../filtered_images` relative to the build directory if not specified.

**Example:**

```bash
./ppxl ../images -e greyscale -o ../processed_output
```

**Stopping the Application:**

Press `e` followed by Enter in the terminal where `ppxl` is running to initiate a graceful shutdown. Alternatively, press `Ctrl+C`.

## Architecture Overview

The application uses a pipeline model with thread-safe queues connecting the stages:

1.  **Watcher Thread:** Monitors the input directory and places image names into `name_queue`.
2.  **Chunker Threads:** Read names from `name_queue`, load images, create chunks, and place them into `chunker_filtering_queue`.
3.  **Filter Threads:** Read chunks from `chunker_filtering_queue`, apply effects, and place processed chunks into `filtering_reconstruction_queue`.
4.  **Reconstruction Thread:** Reads processed chunks from `filtering_reconstruction_queue`, assembles them into final images, and saves them to the output directory.
5.  **Auxiliary Threads:** Manage statistics display and user input for shutdown.
```// filepath: /mnt/d/basp/OS/README.md
# ppxl - Concurrent Image Processing Pipeline

## Description

`ppxl` is a command-line application designed to process images concurrently using a multi-stage pipeline. It monitors an input directory for new images, breaks them into chunks, applies specified image effects using multiple threads, reconstructs the processed images, and saves them to an output directory. The application displays real-time processing statistics and supports graceful shutdown.

## Features

*   **Directory Monitoring:** Continuously watches a specified input directory for new image files.
*   **Multi-threaded Processing:** Utilizes multiple threads for image chunking and filtering stages to leverage multi-core processors.
*   **Pipeline Architecture:** Employs thread-safe queues to pass data between processing stages (Naming -> Chunking -> Filtering -> Reconstruction).
*   **Configurable Effects:** Allows specifying image effects to be applied via command-line arguments.
*   **Real-time Statistics:** Displays the number of images read, written, and discarded during processing.
*   **Graceful Shutdown:** Handles `SIGINT` and `SIGTERM` signals (e.g., via Ctrl+C or user input 'e') to shut down worker threads cleanly.

## Dependencies

*   A C compiler supporting C11 (for `stdatomic.h`)
*   POSIX Threads (`pthread`) library
*   Standard C libraries
*   (Potentially an image loading/saving library like `libpng`, `libjpeg`, or a custom one if `image.h` relies on it - *Add specific library if known*)
*   CMake (for building)

## Building

1.  **Clone the repository (if applicable):**
    ```bash
    git clone <repository_url>
    cd OS # Or your project root
    ```
2.  **Create a build directory:**
    ```bash
    mkdir build
    cd build
    ```
3.  **Run CMake:**
    ```bash
    cmake ..
    ```
    *   To build in Debug mode (includes `PRINTF` statements): `cmake .. -DCMAKE_BUILD_TYPE=Debug`
    *   To build in Release mode (optimized, `PRINTF` disabled): `cmake .. -DCMAKE_BUILD_TYPE=Release`
4.  **Compile:**
    ```bash
    make
    ```
    The executable `ppxl` will be created in the `build` directory.

## Usage

Run the executable from the `build` directory (or after installing it to your PATH):

```bash
./ppxl <input_directory> -e <effects> [-o <output_directory>]
```

**Arguments:**

*   `<input_directory>`: (Required) Path to the directory containing the images to process.
*   `-e <effects>`: (Required) Specifies the image effects to apply (e.g., `"greyscale"`). The exact format depends on the implementation in `chunk_threader.c` / `process_chunk`.
*   `-o <output_directory>`: (Optional) Path to the directory where processed images will be saved. Defaults to `../filtered_images` relative to the build directory if not specified.

**Example:**

```bash
./ppxl ../images -e greyscale -o ../processed_output
```

**Stopping the Application:**

Press `e` followed by Enter in the terminal where `ppxl` is running to initiate a graceful shutdown. Alternatively, press `Ctrl+C`.

## Architecture Overview

The application uses a pipeline model with thread-safe queues connecting the stages:

1.  **Watcher Thread:** Monitors the input directory and places image names into `name_queue`.
2.  **Chunker Threads:** Read names from `name_queue`, load images, create chunks, and place them into `chunker_filtering_queue`.
3.  **Filter Threads:** Read chunks from `chunker_filtering_queue`, apply effects, and place processed chunks into `filtering_reconstruction_queue`.
4.  **Reconstruction Thread:** Reads processed chunks from `filtering_reconstruction_queue`, assembles them into final images, and saves them to the output directory.
5.  **Auxiliary Threads:** Manage statistics display and user input for shutdown.