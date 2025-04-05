#pragma once

#include <cstdint>
#include <functional>
#include <limits>
#include <new>
#include <thread>
#include <utility>

namespace dcpl::rcu {

void flush_callbacks();

void enter();

void exit();

void enqueue_callback(void* data, void (*fn)(void*));

void synchronize();

template <typename T>
void object_release(void* ptr) {
  delete reinterpret_cast<T*>(ptr);
}

template <typename T>
void array_release(void* ptr) {
  delete[] reinterpret_cast<T*>(ptr);
}

template <typename T>
void free_object(T* ptr) {
  enqueue_callback(ptr, &object_release<T>);
}

template <typename T>
void free_array(T* ptr) {
  enqueue_callback(ptr, &array_release<T>);
}

inline void mem_delete(void* ptr) {
  enqueue_callback(ptr, operator delete);
}

struct context {
  context() {
    enter();
  }

  ~context() {
    exit();
  }
};

}

