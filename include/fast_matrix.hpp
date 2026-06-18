#pragma once
#include <vector>
#include <thread>
#include <stdexcept>
#include "allocator.hpp"

template <typename T>
class FastMatrix {
private:
    size_t rows;
    size_t cols;
    std::vector<T, Allocator<T>> data;

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

    size_t get_rows() const { return rows; }
    size_t get_cols() const { return cols; }

    FastMatrix multiply_naive(const FastMatrix& B) const {
        if (cols != B.rows) throw std::invalid_argument("Dimension mismatch");
        FastMatrix result(rows, B.cols);

        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < B.cols; ++j) {
                T sum = 0;
                for (size_t k = 0; k < cols; ++k) {
                    sum += (*this)(i, k) * B(k, j);
                }
                result(i, j) = sum;
            }
        }
        return result;
    }

    FastMatrix multiply_parallel(const FastMatrix& B) const {
        if (cols != B.rows) throw std::invalid_argument("Dimension mismatch");
        FastMatrix result(rows, B.cols);

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