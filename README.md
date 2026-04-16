# Custom C Memory Allocator and Leak Profiler

A thread safe, POSIX compliant dynamic memory allocator written from in C. This bypasses the standard C library (`libc`) to manage virtual memory pages directly via the Unix kernel. 

## Architecture

* **Direct Kernel Interface:** Replaces legacy `sbrk()` calls with `mmap()` to request anonymous, private memory pages directly from the OS.
* **Thread Safety:** Utilizes POSIX threads (`pthread_mutex_t`) to lock the global heap space, preventing race conditions.
* **Strict Alignment:** Enforces 8-byte memory alignment `((size + 7) & ~7)` to maintain CPU cache efficiency and prevent hardware alignment faults.
* **Bidirectional Coalescing:** Uses a Doubly Linked List to merge adjacent free blocks (forward and backward) to actively eliminate external fragmentation.

## Features

* **Macro-Injected Profiler:** Hijacks standard `malloc`/`calloc`/`realloc` calls using C preprocessor macros to inject `__FILE__` and `__LINE__` metadata into hidden block headers.
* **Leak Detection:** Generates real-time heap dumps detailing total requested bytes, active pointers, and the exact origin of leaked memory.
* **Use-After-Free Protection (Memory Poisoning):** Instantly overwrites freed payloads with `0xDF` (Dead Free) to neutralize dangling pointers.
* **Double-Free Interception:** Validates block metadata before freeing, preventing fatal Segmentation Faults.

