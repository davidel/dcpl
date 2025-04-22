#include "dcpl/threadpool.h"

#include "dcpl/env.h"
#include "dcpl/thread.h"

namespace dcpl {
namespace {

struct config {
  std::size_t num_threads = 0;
  std::size_t pool_threads = 0;
};

config parse_config() {
  std::size_t concurrency = std::thread::hardware_concurrency();

  return {
    getenv<std::size_t>("DCPL_NUM_THREADS", concurrency),
    getenv<std::size_t>("DCPL_POOL_THREADS", concurrency) };
}

const config& get_config() {
  static config cfg = parse_config();

  return cfg;
}

std::size_t required_threads(std::size_t num_threads) {
  const config& cfg = get_config();

  return num_threads == 0 ? cfg.num_threads : num_threads;
}

}

threadpool::threadpool(std::size_t num_threads) {
  std::size_t thread_count = required_threads(num_threads);

  threads_.reserve(thread_count);
  for (std::size_t i = 0; i < thread_count; ++i) {
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
  const config& cfg = get_config();

  return new threadpool(cfg.pool_threads);
}

std::size_t effective_num_threads(std::size_t num_threads, std::size_t parallelism) {
  std::size_t thread_count = required_threads(num_threads);

  return std::min(thread_count, parallelism);
}

}
