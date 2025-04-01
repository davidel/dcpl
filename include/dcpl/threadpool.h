#pragma once

#include <condition_variable>
#include <deque>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace dcpl {
namespace detail {

template <typename T>
class queue {
 public:
  void stop() {
    {
      std::lock_guard lg(lock_);

      stopped_ = true;
    }
    cv_.notify_all();
  }

  void push(T elem) {
    {
      std::lock_guard lg(lock_);

      queue_.push_back(std::move(elem));
    }
    cv_.notify_one();
  }

  std::optional<T> pop() {
    std::unique_lock ul(lock_);

    cv_.wait(ul, [this]() { return !queue_.empty() || stopped_; });

    std::optional<T> elem;

    if (!queue_.empty()) {
      elem = std::move(queue_.front());
      queue_.pop_front();
    }

    return elem;
  }

 private:
  std::mutex lock_;
  std::condition_variable cv_;
  std::deque<T> queue_;
  bool stopped_ = false;
};

template <typename T>
class result {
 public:
  result() = default;

  result(result&&) = default;

  explicit result(T value) :
      value_(std::move(value)) {
  }

  explicit result(std::exception_ptr exptr) :
      exptr_(std::move(exptr)) {
  }

  result& operator=(result&& ref) = default;

  T get() {
    if (exptr_ != nullptr) {
      std::rethrow_exception(exptr_);
    }

    return std::move(*value_);
  }

 private:
  std::optional<T> value_;
  std::exception_ptr exptr_;
};

template <typename T>
class multi_result {
 public:
  multi_result(std::size_t size) :
      results_(size),
      assign_tracker_(size, false) {
  }

  std::size_t size() const {
    return results_.size();
  }

  T get(std::size_t i) {
    return results_[i].get();
  }

  void set(std::size_t i, result<T> res) {
    bool done = false;
    {
      std::lock_guard lg(lock_);

      results_[i] = std::move(res);
      if (!assign_tracker_[i]) {
        assign_tracker_[i] = true;
        ++assigned_;
        if (assigned_ == results_.size()) {
          done = true;
        }
      }
    }
    if (done) {
      cv_.notify_all();
    }
  }

  void wait() {
    std::unique_lock ul(lock_);

    cv_.wait(ul, [this]() { return assigned_ == results_.size(); });
  }

 private:
  std::vector<result<T>> results_;
  std::vector<bool> assign_tracker_;
  std::size_t assigned_ = 0;
  std::mutex lock_;
  std::condition_variable cv_;
};

template <typename T, typename F>
result<T> run(const F& fn) {
  try {
    return result<T>(fn());
  } catch (...) {
    return result<T>(std::current_exception());
  }
}

}

class threadpool {
 public:
  using thread_function = std::function<void()>;

  explicit threadpool(std::size_t num_threads = 0);

  ~threadpool();

  void stop();

  void push_work(thread_function thread_fn);

  static threadpool* get();

 private:
  void run();

  static threadpool* create_system_pool();

  std::vector<std::unique_ptr<std::thread>> threads_;
  detail::queue<thread_function> function_queue_;
};

std::size_t effective_num_threads(std::size_t num_threads, std::size_t parallelism);

template <typename I, typename T, typename C>
std::vector<T> map(const std::function<T (C&)>& fn, I start, I end,
                   std::size_t num_threads = 0) {
  std::size_t num_results = std::distance(start, end);
  threadpool pool(effective_num_threads(num_threads, num_results));
  detail::multi_result<T> mresult(num_results);
  std::size_t i = 0;

  for (I it = start; it != end; ++it, ++i) {
    auto& value = *it;

    auto map_fn = [&fn, &mresult, &value, i]() {
      detail::result<T> result = detail::run<T>(
          [&fn, &value]() -> T {
            return fn(value);
          });

      mresult.set(i, std::move(result));
    };

    pool.push_work(std::move(map_fn));
  }

  mresult.wait();

  std::vector<T> results;

  results.reserve(mresult.size());
  for (std::size_t r = 0; r < mresult.size(); ++r) {
    results.push_back(mresult.get(r));
  }

  return results;
}

}
