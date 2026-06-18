#include <benchmark/benchmark.h>
#include <vector>
#include "allocator.hpp"

// 1. Benchmark standard std::malloc
static void BM_SystemMalloc(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<int> vec;
        for (int i = 0; i < 100000; ++i) {
            vec.push_back(i);
        }
        benchmark::DoNotOptimize(vec); 
    }
}
BENCHMARK(BM_SystemMalloc)->Threads(1)->Threads(2)->Threads(4)->Threads(8);

// 2. Benchmark custom allocator
static void BM_CustomMalloc(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<int, Allocator<int>> vec;
        for (int i = 0; i < 100000; ++i) {
            vec.push_back(i);
        }
        benchmark::DoNotOptimize(vec);
    }
}
BENCHMARK(BM_CustomMalloc)->Threads(1)->Threads(2)->Threads(4)->Threads(8);

BENCHMARK_MAIN();