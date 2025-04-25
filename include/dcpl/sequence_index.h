#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <span>
#include <unordered_map>
#include <vector>

#include "dcpl/assert.h"
#include "dcpl/core_utils.h"
#include "dcpl/logging.h"
#include "dcpl/multi_merge_sort.h"
#include "dcpl/sequence.h"
#include "dcpl/suffix_array.h"
#include "dcpl/type_traits.h"
#include "dcpl/types.h"

namespace dcpl::sequence {

template <typename T, typename S>
class sequence_index {
  struct qmatch {
    std::size_t data_pos = 0;
    std::size_t query_pos = 0;
  };

 public:
  using data_type = T;
  using array_type = S;
  using value_type = typename S::value_type;

  struct fuzzy_params {
    double max_edit = 0.75;
    double stop_selection = 0.6;
    std::size_t min_anchors = 3;
    double prob_max = 0.002;
    sequence::edit_costs<double> edit_costs;
  };

  struct fuzzy_match {
    std::size_t query_begin = 0;
    std::size_t query_end = 0;
    std::size_t data_begin = 0;
    std::size_t data_end = 0;
    double distance = 0.0;

    auto operator<=>(const fuzzy_match&) const = default;
  };

  explicit sequence_index(T data, S suffix_array) :
      data_(std::move(data)),
      suffix_array_(std::move(suffix_array)) {
  }

  template <typename V>
  std::vector<fuzzy_match> fuzzy_search(const V& query, const fuzzy_params& params) {
    collect_result cres = collect(query, params);
    std::set<fuzzy_match> matches;
    std::size_t base_index = 0;
    std::size_t prev_base = 0;
    std::size_t snap_base = 0;

    for (std::size_t i = 0; i < cres.qmatches.size(); ++i) {
      std::size_t curr_base = cres.qmatches[i].data_pos - cres.qmatches[i].query_pos;
      std::size_t delta = curr_base - prev_base;

      if (delta > query.size()) {
        auto fmatch = compute_match(query, cres.qmatches, base_index, i,
                                    cres.min_span, params);

        if (fmatch) {
          matches.insert(*fmatch);

          i = base_index;
          ++base_index;
          prev_base = snap_base;
        } else {
          prev_base = curr_base;
          base_index = i;
        }
      } else if (i == base_index + 1) {
        snap_base = curr_base;
      }
    }

    return { matches.begin(), matches.end() };
  }

  void clear_cache() {
    cache_.clear();
  }

 private:
  struct collect_result {
    std::vector<qmatch> qmatches;
    std::size_t query_begin = 0;
    std::size_t query_end = 0;
    std::size_t min_span = 0;
  };

  template <typename V>
  collect_result collect(const V& query, const fuzzy_params& params) {
    struct query_positions {
      std::size_t pos = 0;
      std::span<const std::size_t> positions;
    };

    std::set<value_type> unique_values;
    std::vector<query_positions> positions;

    positions.reserve(query.size());
    for (std::size_t i = 0; i < query.size(); ++i) {
      auto span = get(query[i], params.prob_max);

      if (!span.empty()) {
        positions.emplace_back(i, span);
      }
      unique_values.insert(query[i]);
    }

    std::sort(positions.begin(), positions.end(),
              [](const auto& sp1, const auto& sp2) {
                return sp1.positions.size() < sp2.positions.size();
              });

    std::size_t trim_index =
        static_cast<std::size_t>(params.stop_selection * unique_values.size());

    trim_index = std::max<std::size_t>(trim_index, params.min_anchors);
    trim_index = std::min<std::size_t>(trim_index, positions.size());

    std::vector<bool> dropped(query.size(), true);

    for (std::size_t i = 0; i < trim_index; ++i) {
      dropped[positions[i].pos] = false;
    }

    std::size_t query_begin = 0;
    std::size_t query_end = query.size();

    for (; query_begin < query_end && dropped[query_begin]; ++query_begin) { }
    for (; query_begin < query_end && dropped[query_end - 1]; --query_end) { }

    std::size_t min_span = static_cast<std::size_t>(params.max_edit * query.size());

    DCPL_DLOG() << "*** QS=" << query.size()
                << " MS=" << min_span
                << " SS=" << (query_end - query_begin)
                << " [" << query_begin << ", " << query_end << ")";

    std::vector<std::vector<qmatch>> query_matches;

    for (std::size_t i = 0; i < trim_index; ++i) {
      const query_positions& qpos = positions[i];
      std::vector<qmatch> matches;

      matches.reserve(matches.size() + qpos.positions.size() + 1);
      for (const auto& pos : qpos.positions) {
        matches.emplace_back(pos, qpos.pos);
      }

      query_matches.emplace_back(std::move(matches));
    }

    auto sort_fn = [](const qmatch& q1, const qmatch& q2) {
      std::size_t s1 = q1.data_pos - q1.query_pos;
      std::size_t s2 = q2.data_pos - q2.query_pos;

      return (s1 != s2) ? (s1 < s2) : (q1.query_pos < q2.query_pos);
    };

    using it_type = remove_cvr<decltype(query_matches[0])>::const_iterator;
    using range_type = sort::range<it_type>;

    std::vector<range_type> ranges;

    ranges.reserve(query_matches.size());
    for (const auto& sqm : query_matches) {
      ranges.emplace_back(sqm.begin(), sqm.end());
    }

    std::vector<qmatch> qmatches = sort::multi_merge(ranges, sort_fn);

    return { std::move(qmatches), query_begin, query_end, min_span };
  }

  std::span<const std::size_t> get(value_type key, double prob_max) {
    auto bit = cache_.find(key);

    if (bit == cache_.end()) {
      suffix_array::partition part =
          suffix_array::bounds(data_, suffix_array_, key,
                               { 0, suffix_array_.size(), 0 });
      std::size_t count = part.end - part.begin;
      double prob = static_cast<double>(count) / static_cast<double>(data_.size());
      std::vector<std::size_t> positions;

      if (prob_max >= prob) {
        positions.reserve(count);
        for (std::size_t pos = part.begin; pos != part.end; ++pos) {
          positions.push_back(suffix_array_[pos]);
        }

        std::sort(positions.begin(), positions.end());
      }

      bit = cache_.emplace(key, std::move(positions)).first;
    }

    return { bit->second.data(), bit->second.size() };
  }

  template <typename V, typename Q>
  std::optional<fuzzy_match> compute_match(const V& query, const Q& qmatches,
                                           std::size_t begin, std::size_t end,
                                           std::size_t min_span,
                                           const fuzzy_params& params) {
    std::map<std::size_t, std::size_t> sqmatches;

    for (std::size_t i = begin; i < end; ++i) {
      const qmatch& qm = qmatches[i];
      auto it = sqmatches.emplace(qm.query_pos, qm.data_pos);

      if (!it.second) {
        it.first->second = std::max(it.first->second, qm.data_pos);
      }
    }

    if (sqmatches.empty()) {
      return std::nullopt;
    }

    auto bit = sqmatches.begin();
    auto eit = sqmatches.rbegin();

    std::size_t query_begin = bit->first;
    std::size_t query_end = eit->first + 1;
    std::size_t data_begin = bit->second;
    std::size_t data_end = eit->second + 1;

    while (query_begin > 0 && data_begin > 0 &&
           query[query_begin - 1] == data_[data_begin - 1]) {
      --query_begin;
      --data_begin;
    }
    while (query.size() > query_end && data_.size() > data_end &&
           query[query_end] == data_[data_end]) {
      ++query_end;
      ++data_end;
    }

    if (data_begin >= data_end || min_span > query_end - query_begin) {
      return std::nullopt;
    }

    auto data_span = to_span(data_, data_begin, data_end - data_begin);
    double distance = sequence::levenshtein(data_span, query, params.edit_costs);

    return fuzzy_match{ query_begin, query_end, data_begin, data_end, distance };
  }

  T data_;
  S suffix_array_;
  std::unordered_map<value_type, std::vector<std::size_t>> cache_;
};

}

