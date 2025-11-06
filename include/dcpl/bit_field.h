#include <cstddef>
#include <cstdint>

namespace dcpl {

template <typename T = std::size_t>
struct bit_field {
  using value_type = T;

  constexpr T mask() const {
    return ((static_cast<T>(1) << count) - 1) << pos;
  }

  constexpr T get_value(T value) const {
    return (value & mask()) >> pos;
  }

  constexpr T set_value(T src, T value) const {
    return (src & ~mask()) | ((value << pos) & mask());
  }

  unsigned int pos = 0;
  unsigned int count = 0;
};

}

