#include "malloc.h"
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h> 


#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))

struct block_meta {
    size_t size;
    int is_free;
    const char *file;
    int line;
    struct block_meta *prev;
    struct block_meta *next;
};

#define META_SIZE sizeof(struct block_meta)
void *global_head = NULL;


size_t total_memory_requested = 0;
size_t active_memory_bytes = 0;
int active_allocations = 0;
pthread_mutex_t malloc_global_lock = PTHREAD_MUTEX_INITIALIZER; 


static struct block_meta* request_space(struct block_meta* last, size_t size) {
    size_t total_size = META_SIZE + size;
    void *request = mmap(NULL, total_size, PROT_READ | PROT_WRITE, 
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (request == MAP_FAILED) return NULL; 

    struct block_meta *block = (struct block_meta*)request;
    block->size = size;
    block->is_free = 0;
    block->next = NULL;
    block->prev = last; 
    block->file = NULL;
    block->line = 0;

    if (last) last->next = block;
    return block;
}

static struct block_meta *find_free_block(struct block_meta **last, size_t size) {
    struct block_meta *current = global_head;
    while (current && !(current->is_free && current->size >= size)) {
        *last = current;
        current = current->next;
    }
    return current;
}

void *malloc_track(size_t size, const char *file, int line) {
    if (size <= 0) return NULL;
    

    size_t aligned_size = ALIGN(size); 
    struct block_meta *block;

    pthread_mutex_lock(&malloc_global_lock); 

    if (!global_head) { 
        block = request_space(NULL, aligned_size);
        if (!block) {
            pthread_mutex_unlock(&malloc_global_lock);
            return NULL;
        }
        global_head = block;
    } else {
        struct block_meta *last = global_head;
        block = find_free_block(&last, aligned_size);
        if (!block) { 
            block = request_space(last, aligned_size);
            if (!block) {
                pthread_mutex_unlock(&malloc_global_lock);
                return NULL;
            }
        } else {      
            block->is_free = 0;
        }
    }

    block->file = file;
    block->line = line;
    total_memory_requested += aligned_size;
    active_memory_bytes += aligned_size;
    active_allocations++;

    pthread_mutex_unlock(&malloc_global_lock); 
    return (block + 1); 
}

void free(void *ptr) {
    if (!ptr) return;
    struct block_meta *block_ptr = (struct block_meta*)ptr - 1;
    
    pthread_mutex_lock(&malloc_global_lock);

    if (block_ptr->is_free) {
        printf("\n[!] CRITICAL ERROR: Double Free Detected at %p!\n", ptr);
        pthread_mutex_unlock(&malloc_global_lock);
        return; 
    }
    
    block_ptr->is_free = 1; 
    active_memory_bytes -= block_ptr->size;
    active_allocations--;

    memset(ptr, 0xDF, block_ptr->size); 


    if (block_ptr->next && block_ptr->next->is_free) {
        char *current_end = (char *)block_ptr + META_SIZE + block_ptr->size;
        if (current_end == (char *)block_ptr->next) {
            block_ptr->size += META_SIZE + block_ptr->next->size;
            block_ptr->next = block_ptr->next->next;
            if (block_ptr->next) {
                block_ptr->next->prev = block_ptr;
            }
        }
    }


    if (block_ptr->prev && block_ptr->prev->is_free) {
        char *prev_end = (char *)block_ptr->prev + META_SIZE + block_ptr->prev->size;
        if (prev_end == (char *)block_ptr) {
            block_ptr->prev->size += META_SIZE + block_ptr->size;
            block_ptr->prev->next = block_ptr->next;
            if (block_ptr->next) {
                block_ptr->next->prev = block_ptr->prev;
            }
        }
    }

    pthread_mutex_unlock(&malloc_global_lock);
}

void *calloc_track(size_t num, size_t size, const char *file, int line) {
    size_t total = num * size;
    void *ptr = malloc_track(total, file, line);
    if (ptr) memset(ptr, 0, ALIGN(total));
    return ptr;
}

void *realloc_track(void *ptr, size_t size, const char *file, int line) {
    if (!ptr) return malloc_track(size, file, line);
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    struct block_meta *old_block = (struct block_meta*)ptr - 1;
    size_t aligned_size = ALIGN(size);
    if (old_block->size >= aligned_size) return ptr; 

    void *new_ptr = malloc_track(size, file, line);
    if (!new_ptr) return NULL;

    memcpy(new_ptr, ptr, old_block->size);
    free(ptr);
    return new_ptr;
}

void print_heap_dump() {
    printf("\n=== ALLOCATOR HEAP ===\n");
    struct block_meta *curr = global_head;
    int block_num = 0;

    pthread_mutex_lock(&malloc_global_lock);

    while (curr != NULL) {
        printf("Block %d: Addr: %p | Size: %4zu bytes | Status: %s ", 
                block_num, (void*)curr, curr->size, 
                curr->is_free ? "FREE     " : "ALLOCATED");
        
        if (!curr->is_free && curr->file != NULL) {
            printf("| Owner: %s:%d", curr->file, curr->line);
        }
        printf("\n");
        curr = curr->next;
        block_num++;
    }

    printf("==================================\n");
    printf("=== MEMORY LEAK SUMMARY ===\n");
    printf("Total Lifetime Bytes Requested : %zu bytes\n", total_memory_requested);
    printf("Currently Active Pointers      : %d\n", active_allocations);
    printf("Total Bytes Leaked             : %zu bytes\n", active_memory_bytes);
    
    if (active_allocations == 0) {
        printf("STATUS: NO LEAKS DETECTED.\n");
    } else {
        printf("STATUS: MEMORY LEAK DETECTED.\n");
    }
    printf("==================================\n\n");

    pthread_mutex_unlock(&malloc_global_lock);
}