import requests
import os
import signal
import sys
import time
import uuid
import threading

# --- Configuration ---
NUM_IMAGES = 100
IMAGE_WIDTH = 1920
IMAGE_HEIGHT = 1080
IMAGE_DIR = "./images" # Download images to the stream directory
API_URL = f"https://picsum.photos/{IMAGE_WIDTH}/{IMAGE_HEIGHT}"
# --- End Configuration ---

downloaded_files = []
lock = threading.Lock()
stop_event = threading.Event()

# def create_image_dir(): # No longer needed as we use the stream directory
#     """Creates the directory to store images if it doesn't exist."""
#     if not os.path.exists(IMAGE_DIR):
#         try:
#             os.makedirs(IMAGE_DIR)
#             print(f"Created directory: {IMAGE_DIR}")
#         except OSError as e:
#             print(f"Error creating directory {IMAGE_DIR}: {e}", file=sys.stderr)
#             sys.exit(1)

def download_image(session, index):
    """Downloads a single image."""
    if stop_event.is_set():
        return # Stop if cleanup has started

    try:
        response = session.get(API_URL, stream=True, timeout=10)
        response.raise_for_status() # Raise an exception for bad status codes

        # Generate a unique filename
        filename = f"{uuid.uuid4()}.jpg"
        # os.path.join will correctly handle IMAGE_DIR being "."
        filepath = os.path.join(IMAGE_DIR, filename)

        with open(filepath, 'wb') as f:
            for chunk in response.iter_content(chunk_size=8192):
                if stop_event.is_set():
                    # If stop event is set during download, try to delete partial file
                    try:
                        f.close() # Ensure file handle is closed before deleting
                        os.remove(filepath)
                        print(f"Partially downloaded file deleted: {filepath}")
                    except OSError:
                        pass # Ignore errors if file doesn't exist or can't be deleted
                    return
                f.write(chunk)

        with lock:
            downloaded_files.append(filepath)
        # print(f"Downloaded image {index + 1}/{NUM_IMAGES}: {filepath}")

    except requests.exceptions.RequestException as e:
        print(f"Error downloading image {index + 1}: {e}", file=sys.stderr)
    except IOError as e:
        print(f"Error saving image {index + 1}: {e}", file=sys.stderr)


def download_all_images():
    """Downloads the specified number of images using a session."""
    print(f"Starting download of {NUM_IMAGES} images into the current directory...")
    # Using a session for potential performance benefits (connection reuse)
    with requests.Session() as session:
        # Using threads could speed this up, but let's keep it simple first
        for i in range(NUM_IMAGES):
             if stop_event.is_set():
                 break
             download_image(session, i)
             # Add a small delay to avoid overwhelming the API
             time.sleep(0.1)
    print(f"Finished downloading attempts. {len(downloaded_files)} images saved.")


def cleanup_images(signum=None, frame=None):
    """Deletes all downloaded images."""
    print("\nSignal received. Cleaning up downloaded images...")
    stop_event.set() # Signal download threads to stop

    # Use a copy of the list in case download threads are still modifying it briefly
    with lock:
        files_to_delete = list(downloaded_files)
        # Clear the original list to prevent double deletion attempts
        downloaded_files.clear()

    deleted_count = 0
    for filepath in files_to_delete:
        try:
            # Check existence relative to the current directory
            if os.path.exists(filepath):
                os.remove(filepath)
                deleted_count += 1
                # print(f"Deleted: {filepath}")
        except OSError as e:
            print(f"Error deleting file {filepath}: {e}", file=sys.stderr)

    print(f"Cleanup complete. Deleted {deleted_count} images.")
    # # Attempt to remove the directory if it's empty - Removed as we are using the current directory
    # try:
    #     if os.path.exists(IMAGE_DIR) and not os.listdir(IMAGE_DIR):
    #         os.rmdir(IMAGE_DIR)
    #         print(f"Removed directory: {IMAGE_DIR}")
    # except OSError as e:
    #     print(f"Could not remove directory {IMAGE_DIR} (it might not be empty): {e}", file=sys.stderr)

    sys.exit(0) # Exit gracefully


def main():
    # Register signal handler for Ctrl+C (SIGINT)
    signal.signal(signal.SIGINT, cleanup_images)
    # Register signal handler for termination signal (SIGTERM)
    signal.signal(signal.SIGTERM, cleanup_images)

    # create_image_dir() # No longer needed
    download_all_images()

    if not stop_event.is_set():
        print(f"\nSuccessfully downloaded {len(downloaded_files)} images to the current directory.")
        print("Script is running. Press Ctrl+C to stop and clean up.")

        # Keep the main thread alive waiting for signals
        while not stop_event.is_set():
            time.sleep(1) # Check every second if stop event is set

if __name__ == "__main__":
    main()