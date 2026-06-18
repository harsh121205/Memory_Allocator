#pragma once
#include <unistd.h>
#include <sys/mman.h>
#include <iostream>
#include <cstring>
#include <mutex>


#define ALIGNMENT 64 
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

struct block_meta {
    size_t size;
    int is_free;
    struct block_meta *next;
};

#define META_SIZE sizeof(struct block_meta)

namespace CustomHeap {
    inline std::mutex global_heap_lock;
    
    const size_t TLAB_CHUNK_SIZE = 1024 * 1024; 

    thread_local char* tlab_bump_pointer = nullptr;
    thread_local size_t tlab_remaining_bytes = 0;
    thread_local struct block_meta* local_free_list = nullptr;

    inline void* allocate(size_t size) {
        if (size <= 0) return nullptr;
        size_t aligned_size = ALIGN(size); 
        size_t total_request = META_SIZE + aligned_size;

        struct block_meta *prev = nullptr;
        struct block_meta *curr = local_free_list;
        while (curr) {
            if (curr->size >= aligned_size) {
                if (prev) prev->next = curr->next;
                else local_free_list = curr->next;
                
                curr->is_free = 0;
                return (curr + 1);
            }
            prev = curr;
            curr = curr->next;
        }

        if (tlab_remaining_bytes >= total_request) {
            struct block_meta *block = reinterpret_cast<struct block_meta*>(tlab_bump_pointer);
            block->size = aligned_size;
            block->is_free = 0;
            block->next = nullptr;
            
            tlab_bump_pointer += total_request;
            tlab_remaining_bytes -= total_request;
            return (block + 1);
        }
        std::lock_guard<std::mutex> lock(global_heap_lock); 
        
        size_t alloc_size = (total_request > TLAB_CHUNK_SIZE) ? total_request : TLAB_CHUNK_SIZE;
        void *request = mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, 
                             MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        if (request == MAP_FAILED) return nullptr;

        if (total_request > TLAB_CHUNK_SIZE) {
            struct block_meta *block = static_cast<struct block_meta*>(request);
            block->size = aligned_size;
            block->is_free = 0;
            block->next = nullptr;
            return (block + 1);
        }

        tlab_bump_pointer = static_cast<char*>(request);
        tlab_remaining_bytes = alloc_size;
        struct block_meta *block = reinterpret_cast<struct block_meta*>(tlab_bump_pointer);
        block->size = aligned_size;
        block->is_free = 0;
        block->next = nullptr;
        
        tlab_bump_pointer += total_request;
        tlab_remaining_bytes -= total_request;
        return (block + 1);
    }

    inline void deallocate(void *ptr) {
        if (!ptr) return;
        struct block_meta *block_ptr = static_cast<struct block_meta*>(ptr) - 1;
        
        block_ptr->is_free = 1; 
        
        block_ptr->next = local_free_list;
        local_free_list = block_ptr;
    }
}

template <class T>
struct Allocator {
    typedef T value_type;

    Allocator() = default;
    template <class U> constexpr Allocator(const Allocator<U>&) noexcept {}

    T* allocate(std::size_t n) {
        void* ptr = CustomHeap::allocate(n * sizeof(T));
        if (!ptr) throw std::bad_alloc();
        return static_cast<T*>(ptr);
    }

    void deallocate(T* p, std::size_t n) noexcept {
        CustomHeap::deallocate(p);
    }
};