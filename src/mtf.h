// Squeeze — move-to-front transform over the byte alphabet.
#pragma once

#include <cstdint>
#include <vector>

namespace squeeze {

std::vector<uint8_t> mtfEncode(const std::vector<uint8_t>& input);
std::vector<uint8_t> mtfDecode(const std::vector<uint8_t>& input);

}  // namespace squeeze
