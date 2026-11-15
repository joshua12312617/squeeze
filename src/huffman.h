// Squeeze — canonical Huffman coding over the byte alphabet.
#pragma once

#include <cstdint>
#include <vector>

namespace squeeze {

// Header layout: uint16 symbol count, then (symbol, code length) pairs, then
// the bitstream of canonical codes.
std::vector<uint8_t> huffmanEncode(const std::vector<uint8_t>& input);
std::vector<uint8_t> huffmanDecode(const std::vector<uint8_t>& input);

}  // namespace squeeze
