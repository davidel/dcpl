#pragma once

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <numeric>
#include <span>
#include <vector>

#include "dcpl/assert.h"

namespace dcpl {

template <typename T>
class dyn_tensor {
 public:
  using value_type = T;

  template <typename D>
  explicit dyn_tensor(const D& dims, T init = T{}) :
      dims_(dims.begin(), dims.end()),
      strides_(compute_strides(dims)),
      data_(calc_size(dims), init) {
  }

  explicit dyn_tensor(std::initializer_list<std::size_t> dims, T init = T{}) :
      dims_(dims.begin(), dims.end()),
      strides_(compute_strides(dims)),
      data_(calc_size(dims), init) {
  }

  std::size_t size() const {
    return calc_size(dims_);
  }

  std::size_t size(std::size_t dim) const {
    return dims_.at(dim);
  }

  std::span<const std::size_t> shape() const {
    return { dims_.data(), dims_.size() };
  }

  std::span<const std::size_t> strides() const {
    return { strides_.data(), strides_.size() };
  }

  std::span<const T> data() const {
    return { data_.data(), data_.size() };
  }

  template <typename... ARGS>
  T& operator()(ARGS... args) {
    DCPL_CHECK_EQ(dims_.size(), sizeof...(args));

    return data_[compute_index(args...)];
  }

  template <typename... ARGS>
  const T& operator()(ARGS... args) const {
    DCPL_CHECK_EQ(dims_.size(), sizeof...(args));

    return data_[compute_index(args...)];
  }

 private:
  template <typename I, typename... ARGS>
  static std::size_t dyn_index(std::size_t i, const std::size_t* strides,
                               const std::size_t* dims, const I& v,
                               ARGS... args) {
    std::size_t index = 0;

    if constexpr (sizeof...(args) > 0) {
      index += dyn_index(i + 1, strides, dims, args...);
    }
    DCPL_CHECK_LT(v, dims[i]) << "Index out of bounds";

    index += strides[i] * static_cast<std::size_t>(v);

    return index;
  }

  template <typename... ARGS>
  std::size_t compute_index(ARGS... args) const {
    return dyn_index(0, strides_.data(), dims_.data(), args...);
  }

  template <typename D>
  static std::vector<std::size_t> compute_strides(const D& dims) {
    std::vector<std::size_t> strides;
    std::size_t size = 1;

    for (auto it = std::crbegin(dims); it != std::crend(dims); ++it) {
      strides.push_back(size);
      size *= static_cast<std::size_t>(*it);
    }
    std::reverse(strides.begin(), strides.end());

    return strides;
  }

  template <typename D>
  static std::size_t calc_size(const D& dims) {
    return std::accumulate(dims.begin(), dims.end(), static_cast<std::size_t>(1),
                           std::multiplies<std::size_t>());
  }

  std::vector<std::size_t> dims_;
  std::vector<std::size_t> strides_;
  std::vector<T> data_;
};

}

