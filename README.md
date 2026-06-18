# Multithreaded TLAB Allocator and Matrix Engine

## Overview
This repository contains a C++ memory allocator utilizing Thread-Local Allocation Buffers (TLAB) and a hardware-aware matrix multiplication engine. The project isolates the performance impacts of global mutex locks, CPU cache lines, and spatial locality in multithreaded workloads.

## Architecture Diagram

```text
[Operating System Virtual Memory]
         |
         | (mmap)
         v
[Global Heap] <------------------- (Mutex Protected)
         |
         | (1MB Chunks)
         v
[Thread Local Storage (TLAB)] <--- (Lock-Free Fast Path)
   |-- local_free_list   (Recycled blocks)
   |-- bump_pointer      (Linear allocation)
   |-- remaining_bytes   (Space track)
         |
         | (O(1) Allocation)
         v
[Thread / Application Request]
```

## Allocator API
The allocator exposes a low-level C-style API and a standard C++ allocator trait.

* `void* CustomHeap::allocate(size_t size)`: Requests memory. Checks the thread-local free list first, falls back to the thread-local bump pointer, and finally locks the global heap if the TLAB is exhausted.
* `void CustomHeap::deallocate(void* ptr)`: Pushes the pointer to the front of the calling thread's local free list. No locks are acquired.
* `Allocator<T>`: A template struct satisfying the C++ Named Requirement for `Allocator`. Allows integration with standard library containers.

## Usage Demo
Integrating the allocator with standard C++ containers requires passing the allocator trait as the second template parameter.

```cpp
#include <vector>
#include <iostream>
#include "allocator.hpp"

int main() {
    // Standard vector using the custom lock-free allocator
    std::vector<int, MyVectorAllocator<int>> data;

    // Allocates using the thread-local bump pointer
    for (int i = 0; i < 5; ++i) {
        data.push_back(i * 10);
    }

    for (int val : data) {
        std::cout << val << " ";
    }
    std::cout << "\n";

    // Memory is automatically recycled to the thread's local_free_list upon exit
    return 0;
}
```

## Benchmark Methodology
* **Hardware:** Benchmarks were executed on an 8-core ARM architecture (Apple M-Series) to test multithreaded contention and ARM NEON cache interactions. 
* **Input Sizes:** * *Matrix Multiplication:* Tested on N x N matrices of sizes 100, 250, and 500.
  * *Thrash Test:* Batch sizes of 1,000 allocations per thread, with random sizes uniformly distributed between 8 and 512 bytes.
* **Variance Control:** Google Benchmark was configured to run iterations automatically until statistical variance stabilized, isolating the wall-clock time and CPU time. Thread affinity was managed by the OS scheduler.

## Tests for Correctness and Fragmentation
* **Correctness:** The matrix module includes a pre-benchmark validation step. It computes standard O(N^3) matrix multiplication using the standard system allocator and compares the floating-point results against the custom TLAB allocator. The benchmark aborts if any element deviates by more than 1e-5.
* **Fragmentation Profiling:** Memory fragmentation and overhead are tracked using the OS-level Resident Set Size (RSS). Memory profiles are gathered using `/usr/bin/time -l` during the thrash tests to quantify the exact byte cost of the lock-free speedup.

## Tradeoffs and Failure Cases
This allocator prioritizes execution speed under contention at the explicit expense of memory efficiency.

1. **No Memory Reclamation (munmap):** Deallocated memory is stored in a thread-local linked list for reuse. It is never returned to the operating system. Over time, RSS will strictly monotonically increase or plateau, but never decrease.
2. **Thread Termination Leaks:** If a thread performs allocations, deallocates them to its local free list, and then exits, that memory is orphaned. The global heap cannot access a terminated thread's free list.
3. **Large Allocation Inefficiency:** The allocator requests 1MB chunks to distribute small objects. If the application makes a single massive allocation (e.g., a 10MB matrix), it bypasses the fast path and defaults to a raw `mmap` call. In these cases, the default OS zone allocator is generally faster and more robust.
4. **False Sharing Protection Cost:** All allocations are padded to 64-byte boundaries to match L1 cache lines. Allocating a single 4-byte integer will consume 64 bytes of physical memory to prevent cache invalidation across cores.
