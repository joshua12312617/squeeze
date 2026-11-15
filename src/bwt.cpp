#include "bwt.h"

#include <algorithm>
#include <vector>

namespace squeeze {

namespace {

// Suffix array of `input` plus an implicit sentinel, using prefix doubling.
// Ranks are byte+1 (1..256) so the sentinel (rank 0) is uniquely smallest,
// even when the input contains zero bytes.
std::vector<int> suffixArray(const std::vector<uint8_t>& input) {
  const int n = static_cast<int>(input.size()) + 1;
  std::vector<int> sa(static_cast<size_t>(n));
  std::vector<int> rank(static_cast<size_t>(n));
  std::vector<int> nextRank(static_cast<size_t>(n));
  for (int i = 0; i < n; ++i) {
    sa[static_cast<size_t>(i)] = i;
    rank[static_cast<size_t>(i)] =
        (i < static_cast<int>(input.size()))
            ? static_cast<int>(input[static_cast<size_t>(i)]) + 1
            : 0;
  }
  for (int k = 1; k < n; k <<= 1) {
    auto compare = [&](int i, int j) {
      if (rank[static_cast<size_t>(i)] != rank[static_cast<size_t>(j)]) {
        return rank[static_cast<size_t>(i)] < rank[static_cast<size_t>(j)];
      }
      int ri = (i + k < n) ? rank[static_cast<size_t>(i + k)] : -1;
      int rj = (j + k < n) ? rank[static_cast<size_t>(j + k)] : -1;
      return ri < rj;
    };
    std::sort(sa.begin(), sa.end(), compare);
    nextRank[static_cast<size_t>(sa[0])] = 0;
    for (int i = 1; i < n; ++i) {
      nextRank[static_cast<size_t>(sa[static_cast<size_t>(i)])] =
          nextRank[static_cast<size_t>(sa[static_cast<size_t>(i - 1)])] +
          (compare(sa[static_cast<size_t>(i - 1)], sa[static_cast<size_t>(i)]) ? 1 : 0);
    }
    rank.swap(nextRank);
    if (rank[static_cast<size_t>(sa[static_cast<size_t>(n - 1)])] == n - 1) break;
  }
  return sa;
}

}  // namespace

BwtResult bwtTransform(const std::vector<uint8_t>& input) {
  BwtResult result;
  if (input.empty()) return result;
  std::vector<int> sa = suffixArray(input);
  const int n = static_cast<int>(input.size()) + 1;

  std::vector<uint8_t> lastColumn(static_cast<size_t>(n));
  for (int i = 0; i < n; ++i) {
    int suffix = sa[static_cast<size_t>(i)];
    // Character preceding the rotation start; wraps to the sentinel.
    lastColumn[static_cast<size_t>(i)] =
        (suffix == 0) ? 0  // placeholder; the sentinel row is removed below
                      : input[static_cast<size_t>(suffix - 1)];
  }
  // The appended sentinel is the predecessor of index 0, so the row where
  // the suffix starts at position 0 is the sentinel row — recorded as
  // `primary` and excluded from the serialized last column.
  result.primary = -1;
  result.last.reserve(input.size());
  for (int i = 0; i < n; ++i) {
    if (sa[static_cast<size_t>(i)] == 0) {
      result.primary = i;  // sentinel row, excluded from the output
    } else {
      result.last.push_back(lastColumn[static_cast<size_t>(i)]);
    }
  }
  return result;
}

std::vector<uint8_t> bwtInverse(const BwtResult& result) {
  const int m = static_cast<int>(result.last.size());
  if (m == 0) return {};

  // Rebuild the full last column with the sentinel reinserted at `primary`.
  // Codes: 0 = sentinel, 1..256 = byte value + 1.
  std::vector<int> lastColumn(static_cast<size_t>(m) + 1);
  for (int i = 0; i < result.primary; ++i) {
    lastColumn[static_cast<size_t>(i)] =
        static_cast<int>(result.last[static_cast<size_t>(i)]) + 1;
  }
  lastColumn[static_cast<size_t>(result.primary)] = 0;
  for (int i = result.primary + 1; i <= m; ++i) {
    lastColumn[static_cast<size_t>(i)] =
        static_cast<int>(result.last[static_cast<size_t>(i - 1)]) + 1;
  }

  // Cumulative counts over the ordered alphabet: sentinel first, then bytes.
  std::vector<int> counts(257, 0);
  for (int i = 0; i <= m; ++i) {
    ++counts[static_cast<size_t>(lastColumn[static_cast<size_t>(i)])];
  }
  std::vector<int> first(257, 0);
  int total = 0;
  for (int c = 0; c <= 256; ++c) {
    first[static_cast<size_t>(c)] = total;
    total += counts[static_cast<size_t>(c)];
  }

  // LF mapping: next[i] is the row reached from row i in one backward step.
  std::vector<int> next(static_cast<size_t>(m) + 1);
  std::vector<int> cursor = first;
  for (int i = 0; i <= m; ++i) {
    next[static_cast<size_t>(i)] =
        cursor[static_cast<size_t>(lastColumn[static_cast<size_t>(i)])]++;
  }

  // Traverse the single cycle starting at the sentinel row.  Each step moves
  // one position backwards through the original string, so the characters
  // emerge in reverse — flip the result at the end.
  std::vector<uint8_t> output;
  output.reserve(static_cast<size_t>(m));
  int row = result.primary;
  for (int i = 0; i < m; ++i) {
    row = next[static_cast<size_t>(row)];
    int code = lastColumn[static_cast<size_t>(row)];
    if (code == 0) break;  // sentinel row — only reachable if data is corrupt
    output.push_back(static_cast<uint8_t>(code - 1));
  }
  std::reverse(output.begin(), output.end());
  return output;
}

}  // namespace squeeze
