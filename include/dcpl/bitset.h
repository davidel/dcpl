#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

#include "dcpl/assert.h"
#include "dcpl/core_utils.h"
#include "dcpl/types.h"

namespace dcpl {

class bitset {
 public:
  using map_type = std::uint8_t;

  explicit bitset(std::size_t count) :
      bits_(round_up(count, byte_bits) / byte_bits, 0) {
  }

  std::size_t size() const {
    return bits_.size() * byte_bits;
  }

  std::span<const map_type> data() const {
    return bits_;
  }

  bool is_set(std::size_t pos) const {
    bitpos bpos = getpos(pos);

    return (*bpos.ptr & (1 << bpos.bitno)) != 0;
  }

  bool set(std::size_t pos, bool value) {
    bitpos bpos = getpos(pos);
    bool prev = (*bpos.ptr & (1 << bpos.bitno)) != 0;

    if (value) {
      *bpos.ptr |= 1 << bpos.bitno;
    } else {
      *bpos.ptr &= ~(1 << bpos.bitno);
    }

    return prev;
  }

  void clear() {
    std::fill(bits_.begin(), bits_.end(), 0);
  }

 private:
  static constexpr unsigned int byte_bits = bit_sizeof<map_type>();

  struct bitpos {
    map_type* ptr = nullptr;
    std::size_t bitno = 0;
  };

  bitpos getpos(std::size_t pos) const {
    std::size_t vpos = pos / byte_bits;
    std::size_t bpos = pos % byte_bits;

    DCPL_CHECK_LT(vpos, bits_.size());

    return { const_cast<map_type*>(bits_.data()) + vpos, bpos };
  }

  std::vector<map_type> bits_;
};

}

