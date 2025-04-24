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
  explicit dyn_tensor(const D& dims) :
      dims_(dims.begin(), dims.end()),
      strides_(compute_strides(dims)),
      data_(calc_size(dims)) {
  }

  template <typename V>
  explicit dyn_tensor(const std::initializer_list<V>& dims) :
      dims_(dims.begin(), dims.end()),
      strides_(compute_strides(dims)),
      data_(calc_size(dims)) {
  }

  std::span<const std::size_t> sizes() const {
    return { dims_.data(), dims_.size() };
  }

  std::span<const std::size_t> strides() const {
    return { strides_.data(), strides_.size() };
  }

  std::span<const T> data() const {
    return { data_.data(), data_.size() };
  }

  std::size_t numel() const {
    return calc_size(dims_);
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
  template <typename... ARGS>
  std::size_t compute_index(ARGS... args) const {
    std::size_t index = 0;
    std::size_t i = strides_.size();

    ([&]() {
      DCPL_CHECK_LT(args, dims_[dims_.size() - i]);
      index += static_cast<std::size_t>(args) * strides_[i - 1];
      --i;
    } (), ...);

    return index;
  }

  template <typename D>
  static std::vector<std::size_t> compute_strides(const D& dims) {
    std::vector<std::size_t> strides;
    std::size_t size = 1;

    for (const auto& dim : dims) {
      strides.push_back(size);
      size *= static_cast<std::size_t>(dim);
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

