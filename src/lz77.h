// Squeeze — LZ77 with a 4096-byte sliding window and hash-chain matching.
#pragma once

#include <cstdint>
#include <vector>

namespace squeeze {

std::vector<uint8_t> lz77Encode(const std::vector<uint8_t>& input);
std::vector<uint8_t> lz77Decode(const std::vector<uint8_t>& input);

}  // namespace squeeze
