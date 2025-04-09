#include "dcpl/types.h"
#include "dcpl/utils.h"

namespace dcpl {

template <typename T>
class remaining_time {
 public:
  explicit remaining_time(T count) :
      start_(nstime()),
      count_(count) {
  }

  ns_time estimate(T current_count) const {
    if (current_count >= count_) [[unlikely]] {
      return ns_time{ 0 };
    } else {
      ns_time now = nstime();
      ns_time step = (now - start_) / current_count;

      return step * (count_ - current_count);
    }
  }

 private:
  ns_time start_;
  T count_ = 0;
};

}

