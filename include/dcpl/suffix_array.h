#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <span>
#include <type_traits>
#include <utility>
#include <unordered_set>
#include <vector>

#include "dcpl/core_utils.h"
#include "dcpl/hash.h"
#include "dcpl/logging.h"

namespace dcpl::suffix_array {

template <typename T, typename C>
std::vector<T> compute(const C& data) {
  using size_type = T;
  using value_type = typename C::value_type;

  static_assert(std::is_unsigned_v<value_type>,
                "Input data must have an unsigned type");

  if (data.empty()) {
    return { };
  }

  const size_type n = int_cast<size_type>(data.size());
  const value_type vocab_size = *std::max_element(data.begin(), data.end()) + 1;
  std::vector<size_type> p(n);
  std::vector<size_type> c(n);
  std::vector<size_type> cnt(std::max<size_type>(vocab_size, n));

  for (size_type i = 0; i < n; ++i) {
    cnt[data[i]]++;
  }
  for (size_type i = 1; i < vocab_size; ++i) {
    cnt[i] += cnt[i - 1];
  }
  for (size_type i = 0; i < n; ++i) {
    p[--cnt[data[i]]] = i;
  }

  size_type classes = 1;

  c[p[0]] = 0;
  for (size_type i = 1; i < n; ++i) {
    if (data[p[i]] != data[p[i - 1]]) {
      ++classes;
    }
    c[p[i]] = classes - 1;
  }

  std::vector<size_type> pn(n);
  std::vector<size_type> cn(n);

  for (size_type range = 1; range < n; range <<= 1) {
    for (size_type i = 0; i < n; ++i) {
      if (p[i] >= range) {
        pn[i] = p[i] - range;
      } else {
        pn[i] = p[i] + n - range;
      }
    }
    std::fill(cnt.begin(), cnt.begin() + classes, 0);
    for (size_type i = 0; i < n; ++i) {
      cnt[c[pn[i]]]++;
    }
    for (size_type i = 1; i < classes; ++i) {
      cnt[i] += cnt[i - 1];
    }
    for (size_type i = n; i > 0; --i) {
      p[--cnt[c[pn[i - 1]]]] = pn[i - 1];
    }

    cn[p[0]] = 0;
    classes = 1;
    for (size_type i = 1; i < n; ++i) {
      std::pair cur{ c[p[i]], c[(p[i] + range) % n] };
      std::pair prev{ c[p[i - 1]], c[(p[i - 1] + range) % n] };

      if (cur != prev) {
        ++classes;
      }
      cn[p[i]] = classes - 1;
    }
    c.swap(cn);
  }

  return p;
}

struct partition {
  std::size_t begin = 0;
  std::size_t end = 0;
  std::size_t offset = 0;

  auto operator<=>(const partition&) const = default;
};

template <typename T, typename S, typename V>
partition bounds(const T& data, const S& sa, V value, const partition& part) {
  auto lcomp = [&](auto idx, auto v) {
    if (part.offset + idx >= data.size()) [[unlikely]] {
      return false;
    }
    return data[part.offset + idx] < v;
  };
  auto lit = std::lower_bound(sa.begin() + part.begin, sa.begin() + part.end,
                              value, lcomp);

  if (lit == sa.begin() + part.end) {
    return { part.end, part.end, part.offset };
  }

  auto rcomp = [&](auto v, auto idx) {
    if (part.offset + idx >= data.size()) [[unlikely]] {
      return true;
    }
    return data[part.offset + idx] > v;
  };
  auto eit = std::upper_bound(lit, sa.begin() + part.end, value, rcomp);

  return {
    static_cast<std::size_t>(std::distance(sa.begin(), lit)),
    static_cast<std::size_t>(std::distance(sa.begin(), eit)),
    part.offset
  };
}

template <typename T, typename S, typename V>
partition find(const T& data, const S& sa, const V& seq, const partition& part) {
  std::size_t start = part.begin;
  std::size_t cend = part.end;

  for (std::size_t pos = 0; pos < seq.size(); ++pos) {
    partition cpart{ start, cend, part.offset + pos };
    auto bnd = bounds(data, sa, seq[pos], cpart);

    if (bnd.begin == cend) {
      return { part.end, part.end, part.offset };
    }

    start = bnd.begin;
    cend = bnd.end;
  }

  return { start, cend, part.offset };
}

template <typename T, typename S>
std::vector<partition>
split(const T& data, const S& sa, const partition& part) {
  auto rcomp = [&](auto v, auto idx) {
    if (part.offset + idx >= data.size()) [[unlikely]] {
      return true;
    }
    return data[part.offset + idx] > v;
  };
  std::vector<partition> parts;

  for (std::size_t pos = part.begin; pos != part.end;) {
    std::size_t coffset = part.offset + sa[pos];

    if (data.size() > coffset) [[likely]] {
      typename T::value_type value = data[coffset];
      auto eit = std::upper_bound(sa.begin() + pos, sa.begin() + part.end,
                                  value, rcomp);
      std::size_t end_pos = static_cast<std::size_t>(std::distance(sa.begin(), eit));

      parts.emplace_back(pos, end_pos, part.offset);
      pos = end_pos;
    } else {
      ++pos;
    }
  }

  return parts;
}

struct workpos {
  partition part;
  std::size_t seq_offset = 0;
  std::size_t size = 0;
  double cost = 0.0;
};

struct work_queue {
  struct work_equal {
    bool operator()(const workpos& w1, const workpos& w2) const {
      return w1.part == w2.part && w1.seq_offset == w2.seq_offset &&
          w1.size == w2.size;
    }
  };

  struct work_hash {
    std::size_t operator()(const workpos& work) const {
      return hash(work.part.begin, work.part.end, work.part.offset,
                  work.seq_offset, work.size);
    }
  };

  template<typename... ARGS>
  void push(ARGS&&... args) {
    auto eit = seen.emplace(std::forward<ARGS>(args)...);

    if (eit.second) {
      stack.push_back(&*eit.first);
      std::push_heap(stack.begin(), stack.end(),
                     [](const workpos* w1, const workpos* w2) {
                       return w1->cost > w2->cost;
                     });
    }
  }

  const workpos& pop() {
    const workpos* work = stack.front();

    std::pop_heap(stack.begin(), stack.end(),
                  [](const workpos* w1, const workpos* w2) {
                    return w1->cost > w2->cost;
                  });
    stack.pop_back();

    return *work;
  }

  bool empty() const {
    return stack.empty();
  }

  std::unordered_set<workpos, work_hash, work_equal> seen;
  std::vector<const workpos*> stack;
};

struct fuzzy_match {
  partition part;
  std::size_t size = 0;
  double cost = 0.0;

  template <typename T, typename S, typename F>
  void enum_spans(const T& data, const S& sa, const F& enum_fn) const {
    for (std::size_t pos = part.begin; pos != part.end; ++pos) {
      std::size_t base = sa[pos] + part.offset - size;

      std::span span{ data.data() + base, part.offset + 1 };

      enum_fn(span);
    }
  }
};

struct fuzzy_costs {
  static constexpr double drop_cost = std::numeric_limits<double>::max();

  double skip = drop_cost;
  double replace = drop_cost;
  double insert = drop_cost;
};

using cost_function = std::function<fuzzy_costs(std::size_t, double)>;

template <typename T, typename S, typename V>
std::vector<fuzzy_match>
fuzzy_search(const T& data, const S& sa, const V& seq, const partition& part,
             const cost_function& cost_fn) {
  std::vector<fuzzy_match> matches;
  work_queue work;

  work.push(part, 0, 0, 0.0);
  while (!work.empty()) {
    const workpos& cwork = work.pop();
    partition wpart = bounds(data, sa, seq[cwork.seq_offset], cwork.part);

    if (wpart.begin != wpart.end) {
      if (cwork.seq_offset + 1 == seq.size()) {
        // Completed search. Queue result!
        matches.emplace_back(wpart, cwork.size, cwork.cost);
      } else {
        partition spart(wpart);

        ++spart.offset;
        work.push(spart, cwork.seq_offset + 1, cwork.size + 1, cwork.cost);
      }
    }

    fuzzy_costs costs = cost_fn(cwork.seq_offset, cwork.cost);

    if (costs.skip != fuzzy_costs::drop_cost) {
      if (seq.size() > cwork.seq_offset + 1) {
        work.push(cwork.part, cwork.seq_offset + 1, cwork.size, costs.skip);
      }
    }

    bool replace = (seq.size() > cwork.seq_offset + 1 &&
                    costs.replace != fuzzy_costs::drop_cost);
    bool insert = (costs.insert != fuzzy_costs::drop_cost);

    if (replace || insert) {
      std::vector<partition> splits = split(data, sa, cwork.part);

      if (replace) {
        for (const auto& split_part : splits) {
          partition spart(split_part);

          ++spart.offset;
          work.push(spart, cwork.seq_offset + 1, cwork.size + 1, costs.replace);
        }
      }

      if (insert) {
        for (const auto& split_part : splits) {
          partition spart(split_part);

          ++spart.offset;
          work.push(spart, cwork.seq_offset, cwork.size + 1, costs.insert);
        }
      }
    }
  }

  return matches;
}

}

