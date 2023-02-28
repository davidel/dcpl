#pragma once

#include <memory>
#include <span>
#include <type_traits>
#include <vector>

#include "dcpl/assert.h"

namespace dcpl {

template <typename T>
class storage_span {
 public:
  using value_type = T;
  using vector_type = std::vector<std::remove_cv_t<T>>;

  storage_span() = default;

  storage_span(std::span<T> data) :
      data_(data) {
  }

  storage_span(vector_type&& stg) :
      storage_(std::make_shared<vector_type>(std::move(stg))),
      data_(*storage_) {
  }

  storage_span(const storage_span&) = default;

  storage_span(storage_span&&) = default;

  storage_span& operator=(const storage_span&) = default;

  storage_span& operator=(storage_span&&) = default;

  std::size_t size() const {
    return data_.size();
  }

  T& operator[](std::size_t i) const {
    return data_[i];
  }

  T& at(std::size_t i) const {
    DCPL_CHECK_LT(i, data_.size()) << "Index out of bounds";

    return data_[i];
  }

  std::span<T> data() const {
    return data_;
  }

  const std::shared_ptr<vector_type>& storage() const {
    return storage_;
  }

 private:
  std::shared_ptr<vector_type> storage_;
  std::span<T> data_;
};

}

