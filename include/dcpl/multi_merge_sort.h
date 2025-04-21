#pragma once

#include <cstddef>
#include <functional>
#include <iterator>
#include <queue>
#include <type_traits>
#include <vector>

#include "dcpl/type_traits.h"
#include "dcpl/types.h"

namespace dcpl::sort {

template <typename I>
struct range {
  using iterator_type = I;
  using value_type = typename I::value_type;

  I begin;
  I end;
};

template <typename T, typename C>
std::vector<typename container_type<T>::value_type>
multi_merge(const T& streams, const C& cmp) {
  using container_type = container_type<T>;
  using value_type = typename container_type::value_type;

  auto it_cmp = [&](const container_type& it1, const container_type& it2) {
    return !cmp(*it1.begin, *it2.begin);
  };
  std::size_t count = 0;
  std::priority_queue<container_type,
                      std::vector<container_type>,
                      decltype(it_cmp)> queue(it_cmp);

  for (const auto& stream : streams) {
    queue.push(stream);
    count += std::distance(stream.begin, stream.end);
  }

  std::vector<value_type> merged;

  merged.reserve(count);
  while (!queue.empty()) {
    container_type top = queue.top();

    merged.push_back(*top.begin);

    queue.pop();
    ++top.begin;
    if (top.begin != top.end) {
      queue.push(top);
    }
  }

  return merged;
}

}

