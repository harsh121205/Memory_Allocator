# Multithreaded TLAB Allocator and Cache Aware Matrix Engine

## Overview
A custom C++ memory allocator designed for highly concurrent environments, paired with a hardware aware matrix multiplication engine. This project demonstrates performance tuning by manipulating global locks, CPU cache lines, and thread local storage (TLS).

## The Architecture
* **Thread Local Allocation Buffers (TLAB):** Replaces standard `malloc` with a lock free fast path. Threads request massive 1MB chunks from the OS and allocate internally using private bump pointers.
* **Hardware Alignment:** Enforces 64-byte memory alignment to match modern CPU L1 cache lines, perfectly preventing false sharing across cores.
* **Cache-Aware Matrix Math:** Uses a flat 1D-vector matrix class with a multithreaded `i, k, j` loop order (rather than `i, j, k`) for strictly linear memory access.

## Performance Gains
* **Matrix Math (~20x Speedup):** The `i, k, j` loop order provides perfect spatial locality. The CPU reads memory linearly, eliminating cache misses and fully utilizing pre fetched 64-byte chunks.
* **Memory Thrashing (~8x Speedup):** By bypassing the OS heap's internal mutex, thread contention drops to zero. Allocation is reduced to an `O(1)` pointer addition.

## The Tradeoffs
This system strictly trades RAM efficiency for raw execution speed. It is a **Bump Allocator** and is not meant for general purpose application development:
1. **No Memory Reclamation:** Freed memory is pushed to a local free list but is never unmapped (`munmap`) back to the operating system.
2. **Orphaned Memory Leaks:** Because free lists are `thread_local`, memory freed by a terminating thread is permanently orphaned.
3. **Massive Over Allocation:** A thread needing only 8 bytes will still lock down an entire 1MB chunk of physical RAM from the OS.
4. **Large Allocations:** For single, massive allocations (like allocating a large matrix all at once), the native macOS zone allocator is faster due to OS level virtual memory mapping.
