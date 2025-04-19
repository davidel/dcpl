#pragma once

#include <string>

#include "dcpl/format.h"
#include "dcpl/logging.h"
#include "dcpl/types.h"
#include "dcpl/utils.h"

namespace dcpl {

class timed {
 public:
  explicit timed(std::string msg, int level = logging::DEBUG) :
      msg_(std::move(msg)),
      level_(level),
      ts_(nstime()) {
  }

  ~timed() {
    DCPL_LOG(level_) << msg_ << format_duration(nstime() - ts_);
  }

 private:
  std::string msg_;
  int level_ = logging::DEBUG;
  ns_time ts_ = ns_time{ 0 };
};

}

