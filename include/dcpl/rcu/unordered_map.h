#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iterator>
#include <type_traits>
#include <utility>

#include "dcpl/assert.h"
#include "dcpl/core_utils.h"
#include "dcpl/logging.h"
#include "dcpl/rcu/allocator.h"
#include "dcpl/rcu/pointers.h"
#include "dcpl/rcu/rcu.h"

namespace dcpl::rcu {

template <typename Key, typename T, typename Hash, typename KeyEqual>
class unordered_map;

namespace impl {

template <typename Key, typename T, typename Hash,
          typename KeyEqual = std::equal_to<Key>>
class unordered_map {
  friend class dcpl::rcu::unordered_map<Key, T, Hash, KeyEqual>;

  using atomic_ptr = std::atomic<std::uintptr_t>;

 public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<Key, T>;
  using hasher = Hash;
  using key_equal = KeyEqual;
  using size_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using reference = value_type&;
  using const_reference = const value_type&;
  using pointer = value_type*;
  using const_pointer = const value_type*;

  template <typename V>
  class iterator_impl {
    template <typename>
    friend class iterator_impl;

    template <typename, typename, typename, typename>
    friend class unordered_map;

   public:
    template <typename U, std::enable_if_t<std::is_const_v<V>, U>* = nullptr>
    iterator_impl(const iterator_impl<U>& ref) :
        umap_(ref.umap_),
        kv_(ref.kv_),
        index_(ref.index_) {
    }

    iterator_impl(const iterator_impl&) = default;
    iterator_impl(iterator_impl&&) = default;
    iterator_impl& operator=(const iterator_impl&) = default;

    bool operator==(const iterator_impl& rhs) const {
      return index_ == rhs.index_;
    }

    iterator_impl& operator++() {
      if (index_ != end_index) [[likely]] {
        kv_ = umap_->next_value(&index_);
      }

      return *this;
    }

    iterator_impl operator++(int) {
      iterator_impl it(*this);

      ++it;

      return it;
    }

    V& operator*() const {
      return *kv_;
    }

    V* operator->() const {
      return kv_;
    }

    template <typename U>
    iterator_impl& operator=(U&& value) const {
      kv_ = umap_->install(value_type(kv_->first, std::forward<U>(value)));

      return *this;
    }

   private:
    iterator_impl(unordered_map* umap, V* kv, difference_type index) :
        umap_(umap),
        kv_(kv),
        index_(index) {
    }

    unordered_map* umap_ = nullptr;
    V* kv_ = nullptr;
    difference_type index_ = 0;
  };

  using iterator = iterator_impl<value_type>;
  using const_iterator = iterator_impl<const value_type>;

  ~unordered_map() {
    free_data();
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

  const_iterator begin() const {
    difference_type index = -1;
    value_type* kv = next_value(&index);

    return { const_cast<unordered_map*>(this), kv, index };
  }

  const_iterator end() const {
    return { const_cast<unordered_map*>(this), nullptr, end_index };
  }

  const_iterator find(const key_type& key) const {
    difference_type index = find_key(key);

    if (index < 0) {
      return end();
    }

    std::uintptr_t iptr = data_[index];
    value_type* kv = reinterpret_cast<value_type*>(iptr);

    return { this, kv, index };
  }

  size_type count(const key_type& key) const {
    difference_type index = find_key(key);

    return index >= 0 ? 1 : 0;
  }

  const mapped_type& at(const key_type& key) const {
    difference_type index = find_key(key);

    if (index < 0) {
      throw std::out_of_range("Requested key not found");
    }

    std::uintptr_t iptr = data_[index];
    value_type* kv = reinterpret_cast<value_type*>(iptr);

    return kv->second;
  }

 private:
  static constexpr std::uintptr_t empty_slot = 0;
  static constexpr std::uintptr_t skip_slot = -1;
  static constexpr difference_type end_index = -1;

  explicit unordered_map(size_type size) :
      size_(dcpl::next_prime(size)),
      data_(alloc_data(size_)) {
  }

  template <typename U>
  unordered_map(size_type size, const U& base, const U& end) :
      size_(dcpl::next_prime(size)),
      data_(alloc_data(size_)) {
    for (U it = base; it != end; ++it) {
      insert(*it);
    }
  }

  iterator begin() {
    difference_type index = -1;
    value_type* kv = next_value(&index);

    return { this, kv, index };
  }

  iterator end() {
    return { this, nullptr, end_index };
  }

  iterator find(const key_type& key) {
    difference_type index = find_key(key);

    if (index < 0) {
      return end();
    }

    std::uintptr_t iptr = data_[index];
    value_type* kv = reinterpret_cast<value_type*>(iptr);

    return { this, kv, index };
  }

  mapped_type& at(const key_type& key) {
    difference_type index = find_key(key);

    if (index < 0) {
      throw std::out_of_range("Requested key not found");
    }

    std::uintptr_t iptr = data_[index];
    value_type* kv = reinterpret_cast<value_type*>(iptr);

    return kv->second;
  }

  mapped_type& operator[](const key_type& key) {
    difference_type index = find_key(key);

    if (index < 0) {
      iterator it = insert(value_type{ key, mapped_type() }).first;

      return it->second;
    } else {
      std::uintptr_t iptr = data_[index];
      value_type* kv = reinterpret_cast<value_type*>(iptr);

      return kv->second;
    }
  }

  iterator erase(const key_type& key) {
    difference_type index = find_key(key);

    if (index < 0) {
      return end();
    }

    erase_slot(index);

    value_type* kv = next_value(&index);

    return { this, kv, index };
  }

  iterator erase(const iterator& it) {
    difference_type index = find_key(it->first);

    if (index < 0) {
      return end();
    }

    erase_slot(index);

    value_type* kv = next_value(&index);

    return { this, kv, index };
  }

  template <typename U>
  std::pair<iterator, bool> insert(U&& value) {
    auto [index, exists] = find_slot(value.first);
    value_type* kv = install(index, std::forward<U>(value));
    iterator it{ this, kv, index };

    if (!exists) {
      count_ += 1;
    }

    return { std::move(it), !exists };
  }

  template <typename U>
  value_type* install(difference_type index, U&& value) {
    value_type* kv = allocator<value_type>().allocate(1);

    try {
      new (kv) value_type(std::forward<U>(value));
    } catch (...) {
      allocator<value_type>().deallocate(kv, 1);
    }

    replace_slot(index, kv);

    return kv;
  }

  difference_type find_key(const key_type& key) const {
    difference_type index = static_cast<difference_type>(hasher_(key) % size_);

    for (size_type i = 0; i < size_; ++i) {
      std::uintptr_t iptr = data_[index];

      if (iptr == empty_slot) {
        return -1;
      } else if (iptr != skip_slot) {
        value_type* kv = reinterpret_cast<value_type*>(iptr);

        if (key_equal_(key, kv->first)) {
          return index;
        }
      }

      index += 1;
      if (index >= static_cast<difference_type>(size_)) [[unlikely]] {
        index = 0;
      }
    }

    return -1;
  }

  std::pair<difference_type, bool> find_slot(const key_type& key) const {
    difference_type index = static_cast<difference_type>(hasher_(key) % size_);

    for (size_type i = 0; i < size_; ++i) {
      std::uintptr_t iptr = data_[index];

      if (iptr == empty_slot || iptr == skip_slot) {
        return { index, false };
      }

      value_type* kv = reinterpret_cast<value_type*>(iptr);

      if (key_equal_(key, kv->first)) {
        return { index, true };
      }

      index += 1;
      if (index >= static_cast<difference_type>(size_)) [[unlikely]] {
        index = 0;
      }
    }

    DCPL_THROW() << "Full map slots";
  }

  value_type* next_value(difference_type* index) const {
    difference_type ssize = static_cast<difference_type>(size_);

    for (difference_type i = *index + 1; i < ssize; ++i) {
      std::uintptr_t iptr = data_[i];

      if (iptr != empty_slot && iptr != skip_slot) {
        value_type* kv = reinterpret_cast<value_type*>(iptr);

        *index = i;

        return kv;
      }
    }
    *index = -1;

    return nullptr;
  }

  static atomic_ptr* alloc_data(size_type size) {
    atomic_ptr* data = allocator<atomic_ptr>().allocate(size);

    for (size_type i = 0; i < size; ++i) {
      new (data + i) atomic_ptr(empty_slot);
    }

    return data;
  }

  void replace_slot(size_type index, value_type* kv) {
    std::uintptr_t iptr = reinterpret_cast<std::uintptr_t>(kv);
    std::uintptr_t ipptr = data_[index].exchange(iptr);

    if (ipptr != skip_slot && ipptr != empty_slot) {
      value_type* pkv = reinterpret_cast<value_type*>(ipptr);

      free_object(pkv);
    }
  }

  bool erase_slot(size_type index) {
    std::uintptr_t ipptr = data_[index];

    if (ipptr != skip_slot && ipptr != empty_slot) {
      value_type* pkv = reinterpret_cast<value_type*>(ipptr);

      free_object(pkv);

      data_[index] = skip_slot;
      count_ -= 1;

      return true;
    }

    return false;
  }

  void free_data() {
    for (size_type i = 0; i < size_; ++i) {
      std::uintptr_t iptr = data_[i];

      if (iptr != skip_slot && iptr != empty_slot) {
        value_type* kv = reinterpret_cast<value_type*>(iptr);

        free_object(kv);
      }
    }
    allocator<atomic_ptr>().deallocate(data_, size_);
  }

  double load() const {
    return static_cast<double>(count_) / static_cast<double>(size_);
  }

  hasher hasher_;
  key_equal key_equal_;
  size_type size_ = 0;
  std::atomic<size_type> count_ = 0;
  atomic_ptr* data_ = nullptr;
};

}

// In RCU there is a single writer (eventually serializing among themselves with
// external locks) and multiple readers which do not serialize at all, except for
// accessing data within a section where the rcu::context is held.
// This unordered_map implementation is effectively a flat_map, consisting of an
// array of pointers to the value_type objects (std::pair<Key, T>).
// Readers should grab an rcu::context in scope, and then use the impl_unordered_map
// reference returned by the get() API.
template <typename Key, typename T,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>>
class unordered_map {
  static constexpr std::size_t init_size = 8;
  static constexpr double max_load = 0.75;

 public:
  using impl_unordered_map = impl::unordered_map<Key, T, Hash, KeyEqual>;

  using key_type = impl_unordered_map::key_type;
  using mapped_type = impl_unordered_map::mapped_type;
  using hasher = impl_unordered_map::hasher;
  using key_equal = impl_unordered_map::key_equal;
  using value_type = impl_unordered_map::value_type;
  using size_type = impl_unordered_map::size_type;
  using difference_type = impl_unordered_map::difference_type;
  using reference = impl_unordered_map::reference;
  using const_reference = impl_unordered_map::const_reference;
  using pointer = impl_unordered_map::pointer;
  using const_pointer = impl_unordered_map::const_pointer;
  using iterator = impl_unordered_map::iterator;
  using const_iterator = impl_unordered_map::const_iterator;

  unordered_map() :
      umap_(new impl_unordered_map(init_size)) {
  }

  const impl_unordered_map& get() const {
    return *umap_;
  }

  size_type size() const {
    return umap_->size();
  }

  bool empty() const {
    return umap_->empty();
  }

  void clear() {
    unique_ptr<impl_unordered_map> umap(new impl_unordered_map(umap_->capacity()));

    umap_.swap(umap);
  }

  template <typename U>
  std::pair<iterator, bool> insert(U&& value) {
    if (umap_->load() >= max_load) {
      unique_ptr<impl_unordered_map>
          umap(new impl_unordered_map(2 * umap_->capacity(), umap_->begin(),
                                      umap_->end()));

      umap_.swap(umap);
    }

    return umap_->insert(std::forward<U>(value));
  }

  template <typename... ARGS>
  std::pair<iterator, bool> emplace(ARGS&&... args) {
    return insert(value_type(std::forward<ARGS>(args)...));
  }

  const_iterator begin() const {
    return umap_->begin();
  }

  const_iterator end() const {
    return umap_->end();
  }

  const_iterator find(const key_type& key) const {
    return umap_->find(key);
  }

  iterator find(const key_type& key) {
    return umap_->find(key);
  }

  size_type count(const key_type& key) const {
    return umap_->count(key);
  }

  const mapped_type& at(const key_type& key) const {
    return umap_->at(key);
  }

  mapped_type& at(const key_type& key) {
    return umap_->at(key);
  }

  mapped_type& operator[](const key_type& key) {
    return umap_->operator[](key);
  }

  iterator erase(const key_type& key) {
    return umap_->erase(key);
  }

  iterator erase(const iterator& it) {
    return umap_->erase(it);
  }

  void swap(unordered_map& other) {
    umap_.swap(other.umap_);
  }

 private:
  unique_ptr<impl_unordered_map> umap_;
};

}

