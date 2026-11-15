#include "mtf.h"

#include <array>

namespace squeeze {

std::vector<uint8_t> mtfEncode(const std::vector<uint8_t>& input) {
  std::array<uint8_t, 256> list{};
  for (int i = 0; i < 256; ++i) list[static_cast<size_t>(i)] = static_cast<uint8_t>(i);
  std::vector<uint8_t> output;
  output.reserve(input.size());
  for (uint8_t byte : input) {
    // Find the byte's current index.
    uint8_t index = 0;
    while (list[index] != byte) ++index;
    output.push_back(index);
    // Move it to the front.
    for (int i = index; i > 0; --i) list[static_cast<size_t>(i)] = list[static_cast<size_t>(i - 1)];
    list[0] = byte;
  }
  return output;
}

std::vector<uint8_t> mtfDecode(const std::vector<uint8_t>& input) {
  std::array<uint8_t, 256> list{};
  for (int i = 0; i < 256; ++i) list[static_cast<size_t>(i)] = static_cast<uint8_t>(i);
  std::vector<uint8_t> output;
  output.reserve(input.size());
  for (uint8_t index : input) {
    uint8_t byte = list[index];
    output.push_back(byte);
    for (int i = index; i > 0; --i) list[static_cast<size_t>(i)] = list[static_cast<size_t>(i - 1)];
    list[0] = byte;
  }
  return output;
}

}  // namespace squeeze
