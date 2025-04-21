#include <algorithm>
#include <cstddef>
#include <numeric>
#include <vector>

namespace dcpl {

template <typename T, typename S>
std::size_t levenshtein(const T& s1, const S& s2) {
  std::vector<std::size_t> current(s2.size() + 1);
  std::vector<std::size_t> prev(s2.size() + 1);

  std::iota(prev.begin(), prev.end(), 0);

  for (std::size_t i = 1; i <= s1.size(); ++i) {
    current[0] = i;

    for (std::size_t j = 1; j <= s2.size(); ++j) {
      std::size_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;

      current[j] = std::min({ prev[j] + 1,
          current[j - 1] + 1,
          prev[j - 1] + cost });
    }
    std::swap(prev, current);
  }

  return prev[s2.size()];
}

}

