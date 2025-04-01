#pragma once

#include <algorithm>
#include <chrono>
#include <climits>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <limits>
#include <numeric>
#include <optional>
#include <random>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "dcpl/assert.h"
#include "dcpl/atomic_write.h"
#include "dcpl/constants.h"
#include "dcpl/core_utils.h"
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

rnd_generator create_rnd_generator();

template <typename T>
class rnd_generator_ensure {
 public:
  using generator_type = T;

  explicit rnd_generator_ensure(generator_type* ptr) {
    if (ptr != nullptr) {
      ptr_ = ptr;
    } else {
      std::random_device rd;

      uptr_ = std::make_unique<generator_type>(rd());
      ptr_ = uptr_.get();
    }
  }

  generator_type& get() const {
    return *ptr_;
  }

 private:
  generator_type* ptr_ = nullptr;
  std::unique_ptr<generator_type> uptr_;
};

std::string rand_string(std::size_t size, rnd_generator* rgen = nullptr);

template<typename T, typename G>
std::vector<T> randn(std::size_t count, G* rgen, T mean = 0, T stddev = 1) {
  std::normal_distribution<T> distrib(mean, stddev);
  std::vector<T> values;

  values.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    values.push_back(distrib(*rgen));
  }

  return values;
}

template<typename T, typename G>
std::vector<T> rand(std::size_t count, G* rgen, T rmin = 0, T rmax = 1) {
  std::uniform_real_distribution<T> distrib(rmin, rmax);
  std::vector<T> values;

  values.reserve(count);
  for (std::size_t i = 0; i < count; ++i) {
    values.push_back(distrib(*rgen));
  }

  return values;
}

template<typename T, typename G>
std::size_t choice(const T& probs, G* rgen, bool normalize = false) {
  double scaler = normalize ? 1.0 / sum<double>(probs) : 1.0;

  std::uniform_real_distribution<double> distrib(0.0, 1.0);
  double prob = distrib(*rgen);

  for (std::size_t i = 0; i < probs.size(); ++i) {
    double fprob = probs[i] * scaler;

    if (prob <= fprob) {
      return i;
    }
    prob -= fprob;
  }

  DCPL_CHECK_LE(prob, std::numeric_limits<float>::epsilon())
      << "Likely un-normalized probabilities, left " << prob;

  return probs.size() - 1;
}

template<typename T, typename F>
std::vector<std::size_t> argsort(const T& array, const F& cmp) {
  std::vector<std::size_t> indices = iota(array.size());

  std::sort(indices.begin(), indices.end(),
            [&array, &cmp](std::size_t left, std::size_t right) {
              return cmp(array[left], array[right]);
            });

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
std::vector<B> load_file(const std::string& path, bool binary = true) {
  std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode{};
  std::fstream file = open(path, std::ios::in | mode);
  std::streampos fsize = stream_size(&file);
  std::vector<B> fdata(fsize, 0);

  file.seekg(0);
  file.read(fdata.data(), fsize);

  return fdata;
}

template <typename T>
void store_file(const std::string& path, const T& data, bool binary = true) {
  std::ios::openmode mode = binary ? std::ios::binary : std::ios::openmode{};
  atomic_write awfile(path, mode);

  awfile.file().write(reinterpret_cast<const char*>(data.data()),
                      data.size() * sizeof(data[0]));
  awfile.commit();
}

template <typename S>
std::string to_string(const S& s) {
  return std::string(s.data(), s.size());
}

template <typename T, typename S>
T from_chars(const S& s) {
  using parsed_type = std::conditional<std::is_same_v<T, bool>, int, T>::type;
  parsed_type res{};

  auto ccres = std::from_chars(s.data(), s.data() + s.size(), res);

  DCPL_ASSERT(ccres.ec == std::errc() || ccres.ptr != (s.data() + s.size()))
      << "Unable to convert: value=" << to_string(s);

  return res;
}

template <typename T, typename S>
T to_number(const S& s) {
  return from_chars<T>(s);
}

std::span<const char> to_span(const std::string& str);

std::span<const char> to_span(const char* str);

inline std::chrono::time_point<std::chrono::high_resolution_clock, ns_time>
time_point(ns_time nsecs) {
  return std::chrono::time_point<std::chrono::high_resolution_clock, ns_time>(nsecs);
}

ns_time nstime();

double time();

ns_time to_nsecs(double secs);

double from_nsecs(ns_time nsecs);

void sleep_for(ns_time nsecs);

void sleep_until(ns_time epoch_time);

}

