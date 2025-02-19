#pragma once

#include <algorithm>
#include <climits>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <numeric>
#include <optional>
#include <random>
#include <span>
#include <sstream>
#include <string_view>
#include <type_traits>
#include <vector>

#include "dcpl/assert.h"
#include "dcpl/constants.h"
#include "dcpl/hash.h"
#include "dcpl/stdns_override.h"
#include "dcpl/types.h"

namespace dcpl {

template <typename T>
bitmap create_bitmap(std::size_t size, std::span<const T> indices) {
  bitmap bmap(size, false);

  for (auto ix : indices) {
    bmap[ix] = true;
  }

  return bmap;
}

template <typename T>
std::vector<T> reduce_indices(std::span<const T> indices, const bitmap& bmap) {
  std::vector<T> rindices;

  rindices.reserve(indices.size());
  for (auto ix : indices) {
    if (bmap[ix]) {
      rindices.push_back(ix);
    }
  }

  return rindices;
}

template <typename T = std::size_t>
std::vector<T> iota(std::size_t size, T base = 0) {
  std::vector<T> indices(size);

  std::iota(indices.begin(), indices.end(), base);

  return indices;
}

std::string_view read_line(std::string_view* data);

template<typename T>
std::vector<T> arange(T base, T end, T step = 1) {
  DCPL_ASSERT(step != 0 &&
              ((end > base && step > 0) || (base > end && step < 0)))
      << "Invalid range " << base << " ... " << end << " with step " << step;

  std::vector<T> values;

  values.reserve(static_cast<std::size_t>((end - base) / step) + 1);
  if (base <= end) {
    for (T val = base; val < end; val += step) {
      values.push_back(val);
    }
  } else {
    for (T val = base; val > end; val += step) {
      values.push_back(val);
    }
  }

  return values;
}

template<typename T, typename G>
std::vector<T> randn(std::size_t count, G* rgen, T rmin = 0, T rmax = 1) {
  std::uniform_real_distribution<T> gen(rmin, rmax);
  std::vector<T> values;

  values.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    values.push_back(gen(*rgen));
  }

  return values;
}

template<typename T>
std::vector<std::size_t> argsort(const T& array, bool descending = false) {
  std::vector<std::size_t> indices = iota(array.size());

  if (descending) {
    std::sort(indices.begin(), indices.end(),
              [&array](std::size_t left, std::size_t right) {
                return array[left] > array[right];
              });
  } else {
    std::sort(indices.begin(), indices.end(),
              [&array](std::size_t left, std::size_t right) {
                return array[left] < array[right];
              });
  }

  return indices;
}

template<typename T, typename C, typename U>
std::span<U> take(std::span<T> vec, const C& indices, std::span<U> out) {
  DCPL_ASSERT(indices.size() <= out.size()) << "Buffer too small";
  U* out_ptr = out.data();

  for (auto ix : indices) {
    *out_ptr++ = vec[ix];
  }

  return std::span<U>(out.data(), out_ptr - out.data());
}

template<typename T, typename C>
std::vector<std::remove_cv_t<T>> take(std::span<T> vec, const C& indices) {
  std::vector<std::remove_cv_t<T>> values(indices.size());

  take(vec, indices, std::span(values));

  return values;
}

template<typename T, typename C>
std::vector<T> to_vector(const C& data) {
  return std::vector<T>(data.begin(), data.end());
}

template <typename T, typename C>
std::vector<T> to_vector_cast(const C& data) {
  std::vector<T> dest;

  dest.reserve(data.size());
  for (const auto& value : data) {
    dest.push_back(static_cast<T>(value));
  }

  return std::move(dest);
}

template<typename K, typename V, template<typename, typename, typename...> typename C>
C<V, K> invert_map(const C<K, V>& imap) {
  C<V, K> omap;

  for (const auto& it : imap) {
    omap.emplace(it.second, it.first);
  }

  return omap;
}

template<typename G>
std::vector<std::size_t> resample(std::size_t size, std::size_t count, G* rgen,
                                  bool with_replacement = false) {
  std::vector<std::size_t> indices;

  if (count == consts::all) {
    indices = iota(size);
  } else {
    std::size_t ecount = std::min(count, size);
    bitmap mask(size, false);

    indices.reserve(ecount);
    if (with_replacement) {
      bool invert_count = ecount > size / 2;
      std::size_t xcount = invert_count ? size - ecount : ecount;

      while (xcount > 0) {
        std::size_t ix = static_cast<std::size_t>((*rgen)()) % size;

        if (!mask[ix]) {
          mask[ix] = true;
          --xcount;
        }
      }
      for (std::size_t i = 0; i < size; ++i) {
        if (mask[i] ^ invert_count) {
          indices.push_back(i);
        }
      }
    } else {
      for (std::size_t i = 0; i < ecount; ++i) {
        std::size_t ix = static_cast<std::size_t>((*rgen)()) % size;

        mask[ix] = true;
      }
      for (std::size_t i = 0; i < size; ++i) {
        if (mask[i]) {
          indices.push_back(i);
        }
      }
    }
  }

  return indices;
}

template<typename G>
std::span<std::size_t> resample(std::span<std::size_t> in_indices, std::size_t count, G* rgen,
                                bool with_replacement = false) {
  std::span<std::size_t> indices;

  if (count == consts::all || count >= in_indices.size()) {
    indices = in_indices;
  } else {
    if (with_replacement) {
      std::size_t rspace = in_indices.size() - 1;

      for (std::size_t i = 0; i < count; ++i, --rspace) {
        std::size_t ix = static_cast<std::size_t>((*rgen)()) % rspace;

        std::swap(in_indices[i], in_indices[i + ix + 1]);
      }
      indices = in_indices.subspan(0, count);
    } else {
      std::size_t n = 0;

      for (std::size_t i = 0; i < count; ++i) {
        std::size_t ix = static_cast<std::size_t>((*rgen)()) % in_indices.size();

        if (ix > n) {
          std::swap(in_indices[n], in_indices[ix]);
          ++n;
        }
      }
      indices = in_indices.subspan(0, n);
    }
  }

  return indices;
}

template <typename T, typename F>
void for_each_index(const T& sizes, const F& fn) {
  if (std::none_of(sizes.begin(), sizes.end(), [](auto n) { return n == 0; })) {
    std::vector<std::size_t> ind(sizes.size(), 0);

    for (;;) {
      fn(ind);

      std::size_t i = 0;

      ++ind[i];
      while (ind[i] >= sizes[i]) {
        if (i + 1 >= sizes.size()) {
          return;
        }
        ind[i] = 0;
        ++i;
        ++ind[i];
      }
    }
  }
}

template <typename T, typename E, typename F>
std::vector<T> sequence_comp(const E& ins, const F& fn) {
  std::vector<T> outs;

  outs.reserve(ins.size());
  for (const auto& x : ins) {
    std::optional<T> v = fn(x);

    if (v) {
      outs.push_back(*v);
    }
  }

  return outs;
}

template <typename B>
std::vector<B> load_file(const char* path, bool binary) {
  std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode{};
  std::ifstream in_file(path, /*mode=*/ std::ios::in | mode);

  in_file.seekg(0, std::ios::end);

  auto fsize = in_file.tellg();

  std::vector<B> fdata(fsize, 0);

  in_file.seekg(0);
  in_file.read(fdata.data(), fsize);

  return fdata;
}

template <typename S>
std::string to_string(const S& s) {
  return std::string(s.data(), s.size());
}

template <typename T, typename S>
T from_chars(const S& s, std::enable_if_t<std::is_integral_v<T>, T>* = nullptr) {
  T res{};
  auto ccres = std::from_chars(s.data(), s.data() + s.size(), res);

  DCPL_ASSERT(ccres.ec == std::errc() || ccres.ptr != (s.data() + s.size()))
      << "Unable to convert: value=" << to_string(s);

  return res;
}

// On Mac (at least) the std::from_chars() does not support floating point values
// at the moment.
template <typename T, typename S>
T from_chars(const S& s, std::enable_if_t<std::is_floating_point_v<T>, T>* = nullptr) {
  std::string buf(s.data(), s.data() + s.size());
  char* eptr = nullptr;
  auto value = std::strtold(buf.c_str(), &eptr);

  DCPL_ASSERT(eptr == buf.c_str() + s.size()) << "Unable to convert value: " << buf;

  return static_cast<T>(value);
}

template <typename T, typename S>
T to_number(const S& s) {
  return from_chars<T>(s);
}

std::span<const char> to_span(const std::string& str);

std::span<const char> to_span(const char* str);

}

