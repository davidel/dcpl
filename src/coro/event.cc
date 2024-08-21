#include "dcpl/coro/event.h"

#include <algorithm>

#include "dcpl/coro/utils.h"

namespace dcpl::coro {

void event::set(std::size_t count) {
  std::queue<std::coroutine_handle<>> coros;
  {
    std::unique_lock<std::mutex> lock(lock_);

    count_ += count;
    if (count_ >= trigger_) {
      std::swap(coros_, coros);
    }
  }

  while (!coros.empty()) {
    auto coro = coros.front();

    coros.pop();

    spawn(coro);
  }
}

void event::clear() {
  std::unique_lock<std::mutex> lock(lock_);

  count_ = 0;
}

bool event::do_wait(std::coroutine_handle<> coro) {
  std::unique_lock<std::mutex> lock(lock_);

  if (count_ < trigger_) {
    coros_.emplace(coro);
  }

  return count_ < trigger_;
}

}
