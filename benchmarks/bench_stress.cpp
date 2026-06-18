#include <benchmark/benchmark.h>
#include <vector>
#include <random>
#include "allocator.hpp"


const size_t MIN_ALLOC_SIZE = 8;
const size_t MAX_ALLOC_SIZE = 512;
const int BATCH_SIZE = 1000;

// --- 1. system malloc test ---
static void BM_SystemMalloc_Thrash(benchmark::State& state) {
    thread_local std::mt19937 rng(std::random_device{}());
    thread_local std::uniform_int_distribution<size_t> dist(MIN_ALLOC_SIZE, MAX_ALLOC_SIZE);

    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(BATCH_SIZE);


        for (int i = 0; i < BATCH_SIZE; ++i) {
            size_t random_size = dist(rng);
            void* p = std::malloc(random_size);
            
            if (p) {
                static_cast<char*>(p)[0] = 0xAA; 
                benchmark::DoNotOptimize(p);
                ptrs.push_back(p);
            }
        }

        for (void* p : ptrs) {
            std::free(p);
        }
    }
}
BENCHMARK(BM_SystemMalloc_Thrash)->Threads(1)->Threads(2)->Threads(4)->Threads(8);

// custom TLAB test
static void BM_CustomTLAB_Thrash(benchmark::State& state) {
    thread_local std::mt19937 rng(std::random_device{}());
    thread_local std::uniform_int_distribution<size_t> dist(MIN_ALLOC_SIZE, MAX_ALLOC_SIZE);

    for (auto _ : state) {
        std::vector<void*> ptrs;
        ptrs.reserve(BATCH_SIZE);

        for (int i = 0; i < BATCH_SIZE; ++i) {
            size_t random_size = dist(rng);
            
            void* p = CustomHeap::allocate(random_size);
            
            if (p) {
                static_cast<char*>(p)[0] = 0xAA; 
                benchmark::DoNotOptimize(p);
                ptrs.push_back(p);
            }
        }

        for (void* p : ptrs) {
            CustomHeap::deallocate(p);
        }
    }
}
BENCHMARK(BM_CustomTLAB_Thrash)->Threads(1)->Threads(2)->Threads(4)->Threads(8);

BENCHMARK_MAIN();