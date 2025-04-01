#include "dcpl/periodic_task.h"

#include <exception>

#include "dcpl/logging.h"
#include "dcpl/thread.h"

namespace dcpl {

periodic_task::periodic_task(task_fn fn, ns_time period) :
    fn_(std::move(fn)),
    period_(period),
    thread_(thread::create([this]() { run(); })) {
}

periodic_task::~periodic_task() {
  stop();
  thread_->join();
}

void periodic_task::stop() {
  {
    std::lock_guard guard(mtx_);

    stopped_ = true;
  }
  cond_.notify_all();
}

void periodic_task::run() {
  for (;;) {
    {
      std::unique_lock guard(mtx_);

      cond_.wait_for(guard, period_, [this]() { return stopped_; });

      if (stopped_) {
        break;
      }
    }

    try {
      fn_();
    } catch (const std::exception& ex) {
      DCPL_ELOG() << "Error executing periodic task: " << ex.what();
    }
  }
}

}

