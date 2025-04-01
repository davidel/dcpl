#include "dcpl/threadpool.h"

#include "dcpl/env.h"
#include "dcpl/thread.h"

namespace dcpl {
namespace {

static const std::size_t num_threads = getenv<std::size_t>("DCPL_NUM_THREADS", 0);

}

threadpool::threadpool(std::size_t num_threads) {
  if (num_threads == 0) {
    num_threads = std::thread::hardware_concurrency();
  }
  threads_.reserve(num_threads);
  for (std::size_t i = 0; i < num_threads; ++i) {
    threads_.push_back(thread::create([this]() { run(); }));
  }
}

threadpool::~threadpool() {
  stop();
  for (auto& thread : threads_) {
    thread->join();
  }
}

void threadpool::stop() {
  function_queue_.stop();
}

void threadpool::push_work(thread_function thread_fn) {
  function_queue_.push(std::move(thread_fn));
}

void threadpool::run() {
  for (;;) {
    std::optional<thread_function> thread_fn(function_queue_.pop());

    if (!thread_fn) {
      break;
    }
    (*thread_fn)();
  }
}

threadpool* threadpool::get() {
  static threadpool* pool = create_system_pool();

  return pool;
}

threadpool* threadpool::create_system_pool() {
  return new threadpool(num_threads);
}

std::size_t effective_num_threads(std::size_t num_threads, std::size_t parallelism) {
  if (num_threads == 0) {
    num_threads = std::thread::hardware_concurrency();
  }

  return std::min(num_threads, parallelism);
}

}
