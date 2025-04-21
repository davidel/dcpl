#include <algorithm>
#include <cstddef>
#include <memory>
#include <numeric>

namespace dcpl {

template <typename T, typename S>
std::size_t levenshtein(const T& s1, const S& s2) {
  std::size_t size2 = s2.size();
  std::unique_ptr<std::size_t[]> storage =
      std::make_unique<std::size_t[]>(2 * (size2 + 1));
  std::size_t* prev = storage.get();
  std::size_t* current = prev + (size2 + 1);

  std::iota(prev, current, 0);

  for (std::size_t i = 1; i <= s1.size(); ++i) {
    current[0] = i;

    for (std::size_t j = 1; j <= size2; ++j) {
      std::size_t cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;

      current[j] = std::min({ prev[j] + 1,
          current[j - 1] + 1,
          prev[j - 1] + cost });
    }
    std::swap(prev, current);
  }

  return prev[size2];
}

}

