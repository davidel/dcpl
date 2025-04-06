#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <span>
#include <type_traits>

#include "dcpl/assert.h"
#include "dcpl/logging.h"
#include "dcpl/rcu/allocator.h"
#include "dcpl/rcu/pointers.h"
#include "dcpl/rcu/rcu.h"

namespace dcpl::rcu {

template <typename T>
class vector;

namespace impl {

template <typename T>
class vector : private allocator<T> {
  friend class dcpl::rcu::vector<T>;

 public:
  using value_type = T;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;
  using iterator = pointer;
  using const_iterator = const_pointer;

  ~vector() {
    free_array(created_);
  }

  size_type size() const {
    return count_;
  }

  size_type capacity() const {
    return size_;
  }

  bool empty() const {
    return count_ == 0;
  }

  const T* data() const {
    return ptr_;
  }

  const T& operator[](size_type pos) const {
    return ptr_[pos];
  }

  const T& at(size_type pos) const {
    DCPL_CHECK_LT(pos, count_);

    return ptr_[pos];
  }

  const T& front() const {
    DCPL_ASSERT(count_ != 0);

    return ptr_[0];
  }

  const T& back() const {
    DCPL_ASSERT(count_ != 0);

    return ptr_[count_ - 1];
  }

  const_iterator begin() const {
    return ptr_;
  }

  const_iterator end() const {
    return ptr_ + count_;
  }

 private:
  explicit vector(size_type size) :
      size_(size),
      ptr_(allocator<T>::allocate(size)) {
  }

  vector(size_type size, const T& value) :
      size_(size),
      ptr_(allocator<T>::allocate(size)) {
    size_type pos = 0;

    try {
      for (; pos < size; ++pos) {
        new(ptr_ + pos) T(value);
      }
      count_ = created_ = size;
    } catch (...) {
      free_array(pos);
      throw;
    }
  }

  template <typename U>
  vector(size_type size, U base, U top) :
      size_(size),
      ptr_(allocator<T>::allocate(size)) {
    try {
      insert(base, top);
    } catch (...) {
      free_array(created_);
      throw;
    }
  }

  void resize(size_type size, const T& value) {
    DCPL_CHECK_LE(size, size_);

    if (size > count_) {
      size_type pos = count_;

      try {
        for (; pos < size; ++pos) {
          new(ptr_ + pos) T(value);
        }
        count_ = created_ = size;
      } catch (...) {
        count_ = created_ = pos;
        throw;
      }
    } else {
      count_ = size;
    }
  }

  // Insert in the middle is not possible as that would change existing data non
  // atomically from under readers views, so no stdc++ position iterator.
  template <typename U>
  void insert(U base, U top) {
    size_type count = static_cast<size_type>(top - base);

    DCPL_CHECK_LE(count, size_ - count_);

    size_type pos = count_;

    try {
      for (U ptr = base; ptr < top; ++pos, ++ptr) {
        new(ptr_ + pos) T(*ptr);
      }
      count_ = created_ = pos;
    } catch (...) {
      count_ = created_ = pos;
      throw;
    }
  }

  void resize(size_type size) {
    resize(size, T());
  }

  void push_back(const T& value) {
    new(ptr_ + count_) T(value);
    count_ += 1;
    created_ = count_;
  }

  void push_back(T&& value) {
    new(ptr_ + count_) T(value);
    count_ += 1;
    created_ = count_;
  }

  void pop_back() {
    // We simply drop the size, without actually freeing the existing data, as that
    // would remove it from under readers views.
    count_ -= 1;
  }

  double waste_factor() const {
    return created_ != 0 ?
        static_cast<double>(created_ - count_) / static_cast<double>(created_) : 0.0;
  }

  bool has_shrunk() const {
    return created_ > count_;
  }

  void free_array(size_type count) {
    for (T* cptr = ptr_ + count - 1; cptr >= ptr_; --cptr) {
      cptr->~T();
    }
    allocator<T>::deallocate(ptr_, size_);
  }

  size_type size_ = 0;
  std::atomic<size_type> count_ = 0;
  size_type created_ = 0;
  T* ptr_ = nullptr;
};

}

// In RCU there is a single writer (eventually serializing among themselves with
// external locks) and multiple readers which do not serialize at all, except for
// accessing data within a section where the rcu::context is held.
// A vector (a core vector_type that is) can never shrink, and its data is deleted
// (in an RCU fashion) only when a new version of itself is created.
// Operations which lead to changing existing data (like insert() at positions
// different from end()) will trigger the generation of a new version.
// The stored data is always copied when a new version is generated (data cannot be
// moved as it needs to remain valid for readers holding a given vector_type reference).
// Writers should use the main vector API to modify the vector content, while
// readers should either get an vector_type insteance using the get() API, and use
// that to access the data, or use the span() API to extract the current data range.
// The data, once writtten/stored, cannot be modified, hence there are no non-const
// data accessors and iterators.
// Note that when using the vector_type interface, the size of the vector can
// increase (never decrease) among calls, so patterns like the following should br
// avoided:
//
//   dcpl::rcu::context ctx;
//   T* data = alloc(vector_type->size());
//
//   for (i = 0; i < vector_type->size(); ++i) {
//     data[i] = func((*vector_type)[i]); // data[] overflow is size() increases ...
//   }
//
// While the following is OK:
//
//   dcpl::rcu::context ctx;
//   size_type size = vector_type->size();
//   T* data = alloc(size);
//
//   for (i = 0; i < size(); ++i) {
//     data[i] = func((*vector_type)[i]);
//   }
//
template <typename T>
class vector {
  static constexpr std::size_t init_size = 8;
  static constexpr double max_waste = 0.5;

 public:
  using vector_type = impl::vector<T>;

  using value_type = vector_type::value_type;
  using size_type = vector_type::size_type;
  using difference_type = vector_type::difference_type;
  using reference = vector_type::reference;
  using const_reference = vector_type::const_reference;
  using pointer = vector_type::pointer;
  using const_pointer = vector_type::const_pointer;
  using iterator = vector_type::iterator;
  using const_iterator = vector_type::const_iterator;

  vector() :
      vect_(new vector_type(init_size)) {
  }

  explicit vector(size_type size) :
      vect_(new vector_type(size, T())) {
  }

  vector(size_type size, const T& value) :
      vect_(new vector_type(size, value)) {
  }

  const vector_type& get() const {
    return *vect_;
  }

  std::span<const T> span() const {
    const vector_type* vect = get();

    return { vect->data(), vect->size() };
  }

  size_type size() const {
    return vect_->size();
  }

  size_type capacity() const {
    return vect_->capacity();
  }

  bool empty() const {
    return vect_->empty();
  }

  const T& operator[](size_type pos) const {
    return vect_->operator[](pos);
  }

  const T& at(size_type pos) const {
    return vect_->at(pos);
  }

  const T& front() const {
    return vect_->front();
  }

  const T& back() const {
    return vect_->back();
  }

  // The begin()/end() iterators can be safely used only on the writer side (with
  // proper serialization), the the underlying vector_type instance can change
  // during the iteration.
  const_iterator begin() const {
    return vect_->begin();
  }

  const_iterator end() const {
    return vect_->end();
  }

  void reserve(size_type capacity) {
    if (capacity > vect_->capacity()) {
      unique_ptr<vector_type>
          new_vect(new vector_type(capacity, vect_->data(),
                                   vect_->data() + vect_->size()));

      vect_.swap(new_vect);
    }
  }

  void clear() {
    unique_ptr<vector_type> new_vect(new vector_type(vect_->capacity()));

    vect_.swap(new_vect);
  }

  void push_back(const T& value) {
    if (vect_->capacity() >= vect_->size() + 1 && !vect_->has_shrunk()) {
      vect_->push_back(value);
    } else {
      size_type new_size = 2 * vect_->size() + 1;
      unique_ptr<vector_type>
          new_vect(new vector_type(new_size, vect_->data(),
                                   vect_->data() + vect_->size()));

      new_vect->push_back(value);
      vect_.swap(new_vect);
    }
  }

  void push_back(T&& value) {
    if (vect_->capacity() >= vect_->size() + 1 && !vect_->has_shrunk()) {
      vect_->push_back(value);
    } else {
      size_type new_size = 2 * vect_->size() + 1;
      unique_ptr<vector_type>
          new_vect(new vector_type(new_size, vect_->data(),
                                   vect_->data() + vect_->size()));

      new_vect->push_back(value);
      vect_.swap(new_vect);
    }
  }

  template <typename... ARGS>
  void emplace_back(ARGS&&... args) {
    push_back(T(std::forward<ARGS>(args)...));
  }

  // Note that we take a const_iterator here for position, even though semantically
  // is not correct since the result is to override existing data (if the iterator
  // points in the middle of existing data). But we do not want to expose non-const
  // iterators as those can lead to the idea to overwriting existing data, which is
  // not allowed by RCU.
  template <typename U>
  void insert(const_iterator pos, U base, U top) {
    size_type count = static_cast<size_type>(top - base);
    size_type vpos = static_cast<size_type>(pos - begin());

    if (!vect_->has_shrunk() && vect_->capacity() >= (count + vect_->size()) &&
        vpos == vect_->size()) {
      vect_->insert(base, top);
    } else {
      unique_ptr<vector_type>
          new_vect(new vector_type(vect_->capacity() + count, vect_->data(),
                                   vect_->data() + vpos));

      new_vect->insert(base, top);
      if (size_type end_pos = vpos + count; vect_->size() > end_pos) {
        new_vect->insert(begin() + end_pos, begin() + vect_->size());
      }
      vect_.swap(new_vect);
    }
  }

  void pop_back() {
    if (max_waste > vect_->waste_factor()) {
      vect_->pop_back();
    } else {
      size_type size = vect_->size();

      DCPL_ASSERT(size > 0);

      unique_ptr<vector_type>
          new_vect(new vector_type(vect_->capacity(),
                                   vect_->data(), vect_->data() + size - 1));

      vect_.swap(new_vect);
    }
  }

  void resize(size_type size, const T& value) {
    if ((size <= vect_->size() && max_waste > vect_->waste_factor()) ||
        (size >= vect_->size() && vect_->capacity() >= size && !vect_->has_shrunk())) {
      vect_->resize(size, value);
    } else {
      size_type copy_size = std::min(size, vect_->size());
      unique_ptr<vector_type>
          new_vect(new vector_type(size, vect_->data(), vect_->data() + copy_size));

      if (size > copy_size) {
        new_vect->resize(size, value);
      }
      vect_.swap(new_vect);
    }
  }

  void resize(size_type size) {
    resize(size, T());
  }

 private:
  unique_ptr<vector_type> vect_;
};

}

