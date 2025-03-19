#include "dcpl/bloom_filter.h"

#include <algorithm>
#include <bit>

#include "dcpl/assert.h"
#include "dcpl/core_utils.h"

namespace dcpl {

bloom_filter::bloom_filter(std::size_t count, std::size_t entry_bits) :
    entry_bits_(entry_bits),
    size_(count),
    bits_mod_(estimate_bits(count, entry_bits)),
    bits_(bits_mod_) {
}

bool bloom_filter::check(std::size_t hash) const {
  std::vector<std::size_t> bits = get_bitspos(hash);

  for (const auto& bitno : bits) {
    if (!bits_.is_set(bitno)) {
      return false;
    }
  }

  return true;
}

bool bloom_filter::add(std::size_t hash) {
  std::vector<std::size_t> bits = get_bitspos(hash);
  bool was_present = true;

  for (const auto& bitno : bits) {
    bool hit = bits_.set(bitno, true);

    was_present = was_present && hit;
  }

  if (!was_present) {
    DCPL_CHECK_LT(count_, size_);
    count_ += 1;
  }

  return was_present;
}

void bloom_filter::clear() {
  bits_.clear();
  count_ = 0;
}

std::vector<std::size_t> bloom_filter::get_bitspos(std::size_t hash) const {
  std::vector<std::size_t> bits(entry_bits_);
  int vbits = std::bit_width(bits_mod_);
  std::size_t rhash = hash;

  for (std::size_t i = 0; i < entry_bits_; ++i) {
    bits[i] = (rhash + (rhash >> vbits)) % bits_mod_;
    rhash *= 3077569219;
  }

  return bits;
}

std::size_t bloom_filter::estimate_bits(std::size_t count,
                                        std::size_t entry_bits) {
  std::size_t nbits = count * entry_bits;

  return next_prime(nbits * 3 / 2);
}

}

