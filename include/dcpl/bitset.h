#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

#include "dcpl/assert.h"
#include "dcpl/core_utils.h"
#include "dcpl/types.h"

namespace dcpl {

class bitset {
 public:
  using map_type = std::uint8_t;

  explicit bitset(std::size_t count) :
      bits_(round_up(count, map_type_bits) / map_type_bits, 0) {
  }

  std::size_t size() const {
    return bits_.size() * map_type_bits;
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
  static constexpr unsigned int map_type_bits = bit_sizeof<map_type>();

  struct bitpos {
    map_type* ptr = nullptr;
    std::size_t bitno = 0;
  };

  bitpos getpos(std::size_t pos) const {
    std::size_t vpos = pos / map_type_bits;
    std::size_t bpos = pos % map_type_bits;

    DCPL_CHECK_LT(vpos, bits_.size());

    return { const_cast<map_type*>(bits_.data()) + vpos, bpos };
  }

  std::vector<map_type> bits_;
};

}

