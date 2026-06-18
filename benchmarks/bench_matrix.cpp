#include <benchmark/benchmark.h>
#include "fast_matrix.hpp"

// Initialize with random data
FastMatrix<double> generate_matrix(size_t size) {
    FastMatrix<double> mat(size, size);
    for(size_t i = 0; i < size; ++i) {
        for(size_t j = 0; j < size; ++j) {
            mat(i, j) = (i + j) * 0.5; 
        }
    }
    return mat;
}

// 1. Benchmark naive version
static void BM_MatrixMultiply_Naive(benchmark::State& state) {
    size_t size = state.range(0);
    FastMatrix<double> A = generate_matrix(size);
    FastMatrix<double> B = generate_matrix(size);

    for (auto _ : state) {
        FastMatrix<double> C = A.multiply_naive(B);
        benchmark::DoNotOptimize(C); 
    }
}
BENCHMARK(BM_MatrixMultiply_Naive)->Arg(100)->Arg(250)->Arg(500);

// 2. Benchmark optimized
static void BM_MatrixMultiply_Optimized(benchmark::State& state) {
    size_t size = state.range(0);
    FastMatrix<double> A = generate_matrix(size);
    FastMatrix<double> B = generate_matrix(size);

    for (auto _ : state) {
        FastMatrix<double> C = A.multiply_parallel(B);
        benchmark::DoNotOptimize(C);
    }
}
BENCHMARK(BM_MatrixMultiply_Optimized)->Arg(100)->Arg(250)->Arg(500);

BENCHMARK_MAIN();