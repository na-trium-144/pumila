#pragma once
#include "def.h"
#include <cstddef>
#include <vector>
#include <cassert>

namespace PUMILA_NS {
class Matrix {
    std::vector<double> data;
    std::size_t rows_, cols_;

  public:
    Matrix(std::size_t rows, std::size_t cols)
        : data(rows * cols), rows_(rows), cols_(cols) {}
    double *ptr() { return data.data(); }
    const double *ptr() const { return data.data(); }
    double &at(std::size_t y, std::size_t x) {
        assert(y < rows_ && x < cols_);
        return data.at(y * cols_ + x);
    }
    double at(std::size_t y, std::size_t x) const {
        assert(y < rows_ && x < cols_);
        return data.at(y * cols_ + x);
    }
    template <typename T>
    T *rowPtr(std::size_t y) {
        assert(y < rows_);
        assert(sizeof(T) / sizeof(double) == cols_);
        return reinterpret_cast<T *>(&data.at(y * cols_));
    }
    template <typename T>
    const T *rowPtr(std::size_t y) const {
        assert(y < rows_);
        assert(sizeof(T) / sizeof(double) == cols_);
        return reinterpret_cast<const T *>(&data.at(y * cols_));
    }
    std::size_t rows() const { return rows_; }
    std::size_t cols() const { return cols_; }
};
} // namespace PUMILA_NS
