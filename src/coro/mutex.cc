#include "dcpl/coro/mutex.h"

#include "dcpl/coro/utils.h"

namespace dcpl::coro {

void mutex::unlock() {
  std::unique_lock<std::mutex> lock(lock_);

  if (coros_.empty()) {
    locked_ = false;
  } else {
    auto coro = coros_.front();

    coros_.pop();
    lock.unlock();

    spawn(coro);
  }
}

bool mutex::do_lock(std::coroutine_handle<> coro) {
  std::unique_lock<std::mutex> lock(lock_);
  bool lock_result = locked_;

  if (locked_) {
    coros_.emplace(coro);
  } else {
    locked_ = true;
  }

  return lock_result;
}

}
