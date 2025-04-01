#pragma once

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "dcpl/types.h"

namespace dcpl {

class periodic_task {
 public:
  using task_fn = std::function<void(void)>;

  periodic_task(task_fn fn, ns_time period);

  ~periodic_task();

  void stop();

 private:
  void run();

  task_fn fn_;
  ns_time period_;
  bool stopped_ = false;
  std::mutex mtx_;
  std::condition_variable cond_;
  std::unique_ptr<std::thread> thread_;
};

}

