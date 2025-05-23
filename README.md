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

The executable `ppxl` will be created in the `build` directory after running `make`.

**Option 1: Run directly from the build directory:**

```bash
cd build
./ppxl <input_directory> -e <effects> [-o <output_directory>]
```

**Option 2: Add to PATH and run from anywhere:**

To run `ppxl` like a standard command (e.g., `ls`, `grep`) without `./`, you need to add its directory (`/mnt/d/basp/OS/build`) to your shell's `PATH` environment variable.

*   **Temporarily (Current Terminal Session Only):**
    ```bash
    export PATH="/mnt/d/basp/OS/build:$PATH"
    ppxl <input_directory> -e <effects> [-o <output_directory>]
    ```

*   **Permanently (Recommended for User):**
    1.  Edit your shell's configuration file (e.g., `~/.bashrc` or `~/.zshrc`):
        ```bash
        # If using Bash (default on many Linux systems)
        nano ~/.bashrc
        # If using Zsh
        # nano ~/.zshrc
        ```
    2.  Add this line at the end of the file:
        ```bash
        export PATH="/mnt/d/basp/OS/build:$PATH"
        ```
    3.  Save the file and exit the editor (Ctrl+X, then Y, then Enter in nano).
    4.  Apply the changes:
        ```bash
        # If you edited .bashrc
        source ~/.bashrc
        # If you edited .zshrc
        # source ~/.zshrc
        ```
        (Or simply open a new terminal window).
    5.  Now you can run `ppxl` from any directory:
        ```bash
        ppxl <input_directory> -e <effects> [-o <output_directory>]
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

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
