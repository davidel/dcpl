#include <cstddef>
#include <cstdint>
#include <vector>

#include "dcpl/bitset.h"

namespace dcpl {

class bloom_filter {
 public:
  bloom_filter(std::size_t count, std::size_t entry_bits);

  bool check(std::size_t hash) const;

  bool add(std::size_t hash);

  void clear();

 private:
  std::vector<std::size_t> get_bitspos(std::size_t hash) const;

  static std::size_t estimate_bits(std::size_t count, std::size_t entry_bits);

  std::size_t entry_bits_ = 0;
  std::size_t size_ = 0;
  std::size_t bits_mod_ = 0;
  bitset bits_;
  std::size_t count_ = 0;
};

}

