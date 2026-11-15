// Squeeze — run-length encoding in the classic PackBits format.
#pragma once

#include <cstdint>
#include <vector>

namespace squeeze {

// PackBits: control byte 0..127 means "copy the next n+1 literals",
// 129..255 means "repeat the next byte 257-n times".
std::vector<uint8_t> rleEncode(const std::vector<uint8_t>& input);
std::vector<uint8_t> rleDecode(const std::vector<uint8_t>& input);

}  // namespace squeeze
