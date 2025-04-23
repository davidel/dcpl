#include <algorithm>
#include <cstddef>
#include <memory>
#include <numeric>

#include "dcpl/core_utils.h"

namespace dcpl::sequence {

template <typename T = float>
struct edit_costs {
  using value_type = T;

  T deletion = 1;
  T insertion = 1;
  T substitution = 1;
};

template <typename T, typename S, typename C>
C levenshtein(const T& s1, const S& s2, const edit_costs<C>& costs) {
  std::size_t size2 = s2.size();
  std::unique_ptr<C[]> storage =
      std::make_unique<C[]>(2 * (size2 + 1));
  C* prev = storage.get();
  C* current = prev + (size2 + 1);

  linspace(prev, current, static_cast<C>(0), costs.deletion);

  for (std::size_t i = 1; i <= s1.size(); ++i) {
    current[0] = costs.insertion;

    for (std::size_t j = 1; j <= size2; ++j) {
      C cost = (s1[i - 1] == s2[j - 1]) ? static_cast<C>(0) : costs.substitution;

      current[j] = std::min({ prev[j] + costs.deletion,
          current[j - 1] + costs.insertion,
          prev[j - 1] + cost });
    }
    std::swap(prev, current);
  }

  return prev[size2];
}

}

