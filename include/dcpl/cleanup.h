#pragma once

#include <functional>
#include <vector>

namespace dcpl {

class cleanup {
 public:
  using clean_fn = std::function<void()>;

  explicit cleanup(clean_fn fn) {
    cleanups_.push_back(std::move(fn));
  }

  ~cleanup() {
    for (auto& fn : cleanups_) {
      fn();
    }
  }

  std::size_t push(clean_fn fn) {
    cleanups_.push_back(std::move(fn));

    return cleanups_.size() - 1;
  }

  void reset() {
    cleanups_.clear();
  }

 private:
  std::vector<clean_fn> cleanups_;
};

}

