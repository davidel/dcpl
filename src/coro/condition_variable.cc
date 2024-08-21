#include "dcpl/coro/condition_variable.h"

#include <algorithm>

#include "dcpl/coro/utils.h"

namespace dcpl::coro {

void condition_variable::notify_one() {
  std::unique_lock<std::mutex> lock(lock_);

  if (!coros_.empty()) {
    auto coro = coros_.front();

    coros_.pop();
    lock.unlock();

    spawn(coro);
  }
}

void condition_variable::notify_all() {
  std::queue<std::coroutine_handle<>> coros;
  {
    std::unique_lock<std::mutex> lock(lock_);

    std::swap(coros_, coros);
  }

  while (!coros.empty()) {
    auto coro = coros.front();

    coros.pop();

    spawn(coro);
  }
}

coro<> condition_variable::waiter(std::coroutine_handle<> coro, mutex* mtx,
                                  std::function<bool()> wait_fn) {
  for (;;) {
    co_await mtx->lock();

    if (wait_fn()) {
      break;
    }
    mtx->unlock();

    co_await suspend();
  }

  spawn(coro);
}

void condition_variable::do_wait(std::coroutine_handle<> coro, mutex* mtx,
                                 std::function<bool()> wait_fn) {
  auto wcoro = waiter(coro, mtx, wait_fn);

  do_suspend(wcoro.release());
  mtx->unlock();
}

void condition_variable::do_suspend(std::coroutine_handle<> coro) {
  std::unique_lock<std::mutex> lock(lock_);

  coros_.emplace(coro);
}

}
