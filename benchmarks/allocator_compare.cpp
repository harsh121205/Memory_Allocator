#include <benchmark/benchmark.h>
#include <vector>
#include <thread>
#include <stdexcept>
#include <cassert>
#include <cmath>
#include <iostream>
#include "allocator.hpp"


template <typename T, typename Alloc = std::allocator<T>>
class FastMatrix {
private:
    size_t rows;
    size_t cols;
    std::vector<T, Alloc> data;

public:
    FastMatrix(size_t r, size_t c) : rows(r), cols(c) {
        data.resize(rows * cols, 0);
    }

    inline T& operator()(size_t r, size_t c) {
        return data[r * cols + c];
    }

    inline const T& operator()(size_t r, size_t c) const {
        return data[r * cols + c];
    }

    FastMatrix<T, Alloc> multiply_optimized(const FastMatrix<T, Alloc>& B) const {
        if (cols != B.rows) throw std::invalid_argument("Dimension mismatch");
        FastMatrix<T, Alloc> result(rows, B.cols);

        unsigned int num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        size_t rows_per_thread = rows / num_threads;

        auto worker = [&](size_t start_row, size_t end_row) {
            for (size_t i = start_row; i < end_row; ++i) {
                for (size_t k = 0; k < cols; ++k) {
                    T temp = (*this)(i, k);
                    for (size_t j = 0; j < B.cols; ++j) {
                        result(i, j) += temp * B(k, j);
                    }
                }
            }
        };

        for (unsigned int t = 0; t < num_threads; ++t) {
            size_t start = t * rows_per_thread;
            size_t end = (t == num_threads - 1) ? rows : start + rows_per_thread;
            threads.emplace_back(worker, start, end);
        }

        for (auto& th : threads) {
            th.join();
        }

        return result;
    }
};


using SystemMatrix = FastMatrix<double, std::allocator<double>>;
using CustomMatrix = FastMatrix<double, Allocator<double>>;

void prove_correctness() {
    std::cout << "[*]Correctness Proof\n";
    size_t size = 100;
    
    SystemMatrix sys_A(size, size);
    SystemMatrix sys_B(size, size);
    CustomMatrix cus_A(size, size);
    CustomMatrix cus_B(size, size);

    for(size_t i = 0; i < size; ++i) {
        for(size_t j = 0; j < size; ++j) {
            double val = (i + j) * 0.5;
            sys_A(i, j) = val; sys_B(i, j) = val;
            cus_A(i, j) = val; cus_B(i, j) = val;
        }
    }

    SystemMatrix sys_result = sys_A.multiply_optimized(sys_B);
    CustomMatrix cus_result = cus_A.multiply_optimized(cus_B);

    for(size_t i = 0; i < size; ++i) {
        for(size_t j = 0; j < size; ++j) {
            if (std::abs(sys_result(i, j) - cus_result(i, j)) > 1e-5) {
                std::cerr << "[!]ERROR: Data corrupted by custom allocator!\n";
                exit(1);
            }
        }
    }
    std::cout << "[+] SUCCESS: Custom Allocator output is correct.\n\n";
}

// Benchmark
static void BM_MacSystemAllocator(benchmark::State& state) {
    size_t size = state.range(0);
    SystemMatrix A(size, size);
    SystemMatrix B(size, size);

    for (auto _ : state) {
        SystemMatrix C = A.multiply_optimized(B);
        benchmark::DoNotOptimize(C);
    }
}
BENCHMARK(BM_MacSystemAllocator)->Arg(250)->Arg(500)->Arg(750);

static void BM_CustomTLABAllocator(benchmark::State& state) {
    size_t size = state.range(0);
    CustomMatrix A(size, size);
    CustomMatrix B(size, size);

    for (auto _ : state) {
        CustomMatrix C = A.multiply_optimized(B);
        benchmark::DoNotOptimize(C);
    }
}
BENCHMARK(BM_CustomTLABAllocator)->Arg(250)->Arg(500)->Arg(750);

int main(int argc, char** argv) {
    prove_correctness();
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
    ::benchmark::Shutdown();
    return 0;
}