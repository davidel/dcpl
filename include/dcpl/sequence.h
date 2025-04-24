#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <numeric>
#include <vector>

#include "dcpl/core_utils.h"
#include "dcpl/dyn_tensor.h"

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

enum class edit_mode {
  insert,
  remove,
  substitute,
  noop
};

struct edit_operation {
  edit_mode mode = edit_mode::noop;
  std::size_t pos1 = 0;
  std::size_t pos2 = 0;
};

template <typename T, typename S>
std::vector<edit_operation> compute_edits(const T& s1, const S& s2) {
  std::size_t m = s1.size();
  std::size_t n = s2.size();
  dyn_tensor<std::size_t> dp({ m + 1, n + 1 });

  for (std::size_t i = 0; i <= m; ++i) {
    dp(i, 0) = i;
  }
  for (std::size_t j = 0; j <= n; ++j) {
    dp(0, j) = j;
  }
  for (std::size_t i = 1; i <= m; ++i) {
    for (std::size_t j = 1; j <= n; ++j) {
      if (s1[i - 1] == s2[j - 1]) {
        dp(i, j) = dp(i - 1, j - 1);
      } else {
        dp(i, j) = 1 + std::min({ dp(i - 1, j), dp(i, j - 1), dp(i - 1, j - 1) });
      }
    }
  }

  std::vector<edit_operation> operations;

  for (std::size_t i = m, j = n; i > 0 || j > 0;) {
    if (i > 0 && j > 0 && s1[i - 1] == s2[j - 1]) {
      i--;
      j--;
    } else if (j > 0 && dp(i, j) == dp(i, j - 1) + 1) {
      operations.emplace_back(edit_mode::insert, i, j - 1);
      j--;
    } else if (i > 0 && dp(i, j) == dp(i - 1, j) + 1) {
      operations.emplace_back(edit_mode::remove, i - 1);
      i--;
    } else if (i > 0 && j > 0 && dp(i, j) == dp(i - 1, j - 1) + 1) {
      operations.emplace_back(edit_mode::substitute, i - 1, j - 1);
      i--;
      j--;
    }
  }

  return operations;
}

}

