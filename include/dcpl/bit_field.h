#include <cstddef>
#include <cstdint>

namespace dcpl {

template <typename T = std::size_t>
struct bit_field {
  using value_type = T;

  T mask() const {
    return ((static_cast<T>(1) << count) - 1) << pos;
  }

  T get(T value) const {
    return (value & mask()) >> pos;
  }

  T set(T src, T value) const {
    return (src & ~mask()) | ((value << pos) & mask());
  }

  unsigned int pos = 0;
  unsigned int count = 0;
};

template <typename T = std::size_t>
struct bit_setter {
  using value_type = T;

  explicit bit_setter(T init = 0) : value(init) { }

  template <typename U>
  T set(const bit_field<U>& field, T fvalue) {
    value = field.set(value, fvalue);
    mask |= field.mask();

    return value;
  }

  T update(T input_value) const {
    return (input_value & ~mask) | value;
  }

  T value = 0;
  T mask = 0;
};

}

