// Squeeze — the file format and the composed codec pipelines.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace squeeze {

enum class Method : uint8_t {
  kRle = 0,
  kHuffman = 1,
  kLz77 = 2,
  kBwt = 3,  // BWT -> MTF -> RLE -> Huffman, the bzip2-style chain
};

const char* methodName(Method method);

// Encodes `input` with `method` and wraps it in the SQZ1 container.
std::vector<uint8_t> compress(Method method, const std::vector<uint8_t>& input);

// Detects the method from the container header and decodes the payload.
std::vector<uint8_t> decompress(const std::vector<uint8_t>& input);

}  // namespace squeeze
