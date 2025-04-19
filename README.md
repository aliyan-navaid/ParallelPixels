# Project Workload Distribution

## (1) Image loader and buffering (AFFAN)
  - Read images continously from a directory
  - Break images into chunks
    - Multithreaded chunking process
  - (Optional) Use thread pooling to avoid system overload
    - Implement an algorithm for best thread pool based on workload 
    - Make use of queuing until threads are free
  - Write the chunks into a shared memory buffer (shared between #1 & #2)
    - Ensure Synchronization
      - Mutexes
      - Semaphores
  - Implement atleast one function in the filtering procedure 
## (2) Image reading and filtering procedure - prototype only (ALIYAN)
  - Read from the shared buffer
    - Ensure synchronization
      - Mutexes
      - Semaphores
    - Create threads for each chunk read
  - Send them to the filtering pipeline (Sequence filter applying functions)
      - Create the prototypes of the filtering functions
      - Create the basic flow of filtering procedure
  - After the filtering procedure, write the chunks into another shared buffer (shared between #2 & #3)
    - Ensure Synchronization
      - Mutexes
      - Semaphores
  - Implement atleast one function in the filtering procedure
  - (Optional) Thread pooling
## (3) Image reconstruction (FAROOQ)
  - Read the chunks from the 2nd shared buffer
    - Ensure Synchronization
      - Mutexes
      - Semaphores
  - Reconstruct the images based on image IDs and coordinates
  - Write the images back into a directory
  - Implement atleast one function in the filtering procedure
  - (Optional) Thread pooling

## Why not create a thread for each chunk, filter chunk in that thread, and reconstruct chunks sequentially? 