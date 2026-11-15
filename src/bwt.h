// Squeeze — Burrows–Wheeler transform via suffix arrays (prefix doubling).
#pragma once

#include <cstdint>
#include <vector>

namespace squeeze {

struct BwtResult {
  std::vector<uint8_t> last;  // last column without the implicit sentinel
  int primary = 0;            // row of the sentinel in the rotation matrix
};

BwtResult bwtTransform(const std::vector<uint8_t>& input);
std::vector<uint8_t> bwtInverse(const BwtResult& result);

}  // namespace squeeze
