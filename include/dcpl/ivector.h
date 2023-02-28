#pragma once

#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <ostream>
#include <span>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "dcpl/assert.h"

namespace dcpl {
namespace {

template <typename T, std::size_t N>
class iv_storage {

  struct alignas(T) stg {
    char data[N * sizeof(T)];
  };

 public:
  using value_type = T;
  using cvalue_type = std::add_const_t<T>;
  using pointer = T*;
  using const_pointer = cvalue_type*;
  using reference = T&;
  using const_reference = cvalue_type&;

  iv_storage() = default;

  iv_storage(const iv_storage& ref) {
    for (std::size_t i = 0; i < ref.size(); ++i) {
      push_back(ref[i]);
    }
  }

  iv_storage(iv_storage&& ref) {
    for (std::size_t i = 0; i < ref.size(); ++i) {
      push_back(std::move(ref[i]));
    }
  }

  ~iv_storage() {
    for (std::size_t i = size_; i > 0; --i) {
      pop_back();
    }
  }

  std::size_t size() const {
    return size_;
  }

  pointer data() {
    return reinterpret_cast<pointer>(stg_.data);
  }

  const_pointer data() const {
    return const_cast<iv_storage*>(this)->data();
  }

  reference operator[](std::size_t i) {
    return *(data() + i);
  }

  const_reference operator[](std::size_t i) const {
    return const_cast<iv_storage*>(this)->operator[](i);
  }

  reference at(std::size_t i) {
    DCPL_CHECK_LT(i, size_) << "Index out of bounds";

    return *(data() + i);
  }

  const_reference at(std::size_t i) const {
    return const_cast<iv_storage*>(this)->at(i);
  }

  void resize(std::size_t count, const_reference value = value_type()) {
    DCPL_CHECK_LE(count, N);

    if (count <= size_) {
      std::size_t ndrops = size_ - count;

      for (std::size_t i = 0; i < ndrops; ++i) {
        pop_back();
      }
    } else {
      std::size_t nins = count - size_;

      for (std::size_t i = 0; i < nins; ++i) {
        push_back(value);
      }
    }
  }

  void push_back(value_type value) {
    DCPL_CHECK_LT(size_, N);

    new (data() + size_) value_type(std::move(value));

    ++size_;
  }

  template <typename... V>
  reference emplace_back(V&&... args) {
    DCPL_CHECK_LT(size_, N);

    pointer nptr = data() + size_;

    new (nptr) value_type(std::forward<V>(args)...);

    ++size_;

    return *nptr;
  }

  void pop_back() {
    DCPL_CHECK_GT(size_, 0);
    operator[](--size_).~value_type();
  }

 private:
  std::size_t size_ = 0;
  stg stg_;
};

}

template <typename T, std::size_t N>
class ivector {
 public:
  using value_type = T;
  using cvalue_type = std::add_const_t<T>;
  using pointer = T*;
  using const_pointer = cvalue_type*;
  using reference = T&;
  using const_reference = cvalue_type&;

  using iterator = pointer;
  using const_iterator = const_pointer;

  ivector() {
    stg_.template emplace<0>();
  }

  template <typename U>
  explicit ivector(const std::initializer_list<U>& args) {
    args_init(args);
  }

  template <typename U>
  explicit ivector(const std::span<U>& args) {
    args_init(args);
  }

  template <typename U>
  explicit ivector(const std::vector<U>& args) {
    args_init(args);
  }

  ivector(ivector&&) = default;

  ivector(const ivector&) = default;

  ivector& operator=(const ivector&) = default;

  operator std::span<cvalue_type>() const {
    return { data(), size() };
  }

  operator std::span<value_type>() {
    return { data(), size() };
  }

  void resize(std::size_t count, const_reference value = value_type()) {
    storage* ivs = std::get_if<storage>(&stg_);

    if (ivs != nullptr) {
      if (count <= N) {
        ivs->resize(count, value);
        return;
      }
      make_vector(ivs, /*size_hint=*/ count);
    }
    std::get<1>(stg_).resize(count, value);
  }

  void reserve(std::size_t count) {
    storage* ivs = std::get_if<storage>(&stg_);

    if (ivs != nullptr) {
      if (count <= N - ivs->size()) {
        return;
      }
      make_vector(ivs, /*size_hint=*/ count + ivs->size());
    }
    std::get<1>(stg_).reserve(count);
  }

  std::size_t size() const {
    const storage* ivs = std::get_if<storage>(&stg_);

    return ivs != nullptr ? ivs->size() : std::get<1>(stg_).size();
  }

  bool empty() const {
    return size() == 0;
  }

  std::size_t capacity() const {
    const storage* ivs = std::get_if<storage>(&stg_);

    return ivs != nullptr ? N - ivs->size() : std::get<1>(stg_).capacity();
  }

  pointer data() {
    storage* ivs = std::get_if<storage>(&stg_);

    return ivs != nullptr ? ivs->data() : std::get<1>(stg_).data();
  }

  const_pointer data() const {
    return const_cast<ivector*>(this)->data();
  }

  reference operator[](std::size_t i) {
    storage* ivs = std::get_if<storage>(&stg_);

    return ivs != nullptr ? ivs->operator[](i) : std::get<1>(stg_)[i];
  }

  const_reference operator[](std::size_t i) const {
    return const_cast<ivector*>(this)->operator[](i);
  }

  reference at(std::size_t i) {
    storage* ivs = std::get_if<storage>(&stg_);

    return ivs != nullptr ? ivs->at(i) : std::get<1>(stg_).at(i);
  }

  const_reference at(std::size_t i) const {
    return const_cast<ivector*>(this)->at(i);
  }

  void push_back(const_reference value) {
    storage* ivs = std::get_if<storage>(&stg_);

    if (ivs != nullptr) {
      if (ivs->size() < N) {
        ivs->push_back(value);
        return;
      }
      make_vector(ivs, /*size_hint=*/ ivs->size() + 1);
    }
    std::get<1>(stg_).push_back(value);
  }

  template <typename... V>
  reference emplace_back(V&&... args) {
    storage* ivs = std::get_if<storage>(&stg_);

    if (ivs != nullptr) {
      if (ivs->size() < N) {
        return ivs->emplace_back(std::forward<V>(args)...);
      }
      make_vector(ivs, /*size_hint=*/ ivs->size() + 1);
    }

    iv_vector& ivd = std::get<1>(stg_);

    ivd.emplace_back(std::forward<V>(args)...);

    return ivd.back();
  }

  template <typename II>
  void insert(iterator pos, const II& from, const II& to) {
    std::size_t count = std::distance(from, to);
    std::size_t index = std::distance(begin(), pos);
    std::size_t new_size = size() + count;

    resize(new_size);

    std::size_t to_move = size() - index;
    pointer dptr = data();

    for (std::size_t i = 0; i < to_move; ++i) {
      dptr[new_size - 1 - i] = std::move(dptr[index + i]);
    }

    II iptr = from;

    for (std::size_t i = 0; i < count; ++i) {
      dptr[index + i] = *iptr++;
    }
  }

  iterator begin() {
    storage* ivs = std::get_if<storage>(&stg_);

    return ivs != nullptr ? ivs->data() : std::get<1>(stg_).data();
  }

  const_iterator begin() const {
    return const_cast<ivector*>(this)->begin();
  }

  iterator end() {
    storage* ivs = std::get_if<storage>(&stg_);

    if (ivs != nullptr) {
      return ivs->data() + ivs->size();
    }

    iv_vector& ivd = std::get<1>(stg_);

    return ivd.data() + ivd.size();
  }

  const_iterator end() const {
    return const_cast<ivector*>(this)->end();
  }

  bool is_array() const {
    return std::holds_alternative<storage>(stg_);
  }

  friend std::ostream& operator<<(std::ostream& os, const ivector& vec) {
    os << "(";

    std::size_t count = 0;

    for (const auto& sv : vec) {
      if (count++ > 0) {
        os << ", ";
      }
      os << sv;
    }
    os << ")";

    return os;
  }

 private:
  using storage = iv_storage<value_type, N>;
  using iv_vector = std::vector<value_type>;

  template <typename U>
  void args_init(const U& args) {
    if (args.size() > N) {
      iv_vector dvec;

      dvec.reserve(args.size());
      dvec.insert(dvec.end(), args.begin(), args.end());
      stg_.template emplace<1>(std::move(dvec));
    } else {
      storage sarr;

      for (const auto& v : args) {
        sarr.push_back(v);
      }

      stg_.template emplace<0>(std::move(sarr));
    }
  }

  void make_vector(storage* ivs, std::size_t size_hint = 0) {
    iv_vector dvec;

    dvec.reserve(size_hint > 0 ? size_hint : ivs->size());
    for (std::size_t i = 0; i < ivs->size(); ++i) {
      dvec.push_back(std::move((*ivs)[i]));
    }

    stg_.template emplace<1>(std::move(dvec));
  }

  std::variant<storage, iv_vector> stg_;
};

}

