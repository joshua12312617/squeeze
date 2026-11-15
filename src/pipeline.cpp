#include "pipeline.h"

#include <stdexcept>

#include "bitio.h"
#include "bwt.h"
#include "huffman.h"
#include "lz77.h"
#include "mtf.h"
#include "rle.h"

namespace squeeze {

namespace {

const uint8_t kMagic[4] = {'S', 'Q', 'Z', '1'};

uint32_t readU32(const std::vector<uint8_t>& data, size_t offset) {
  return static_cast<uint32_t>(data[offset]) |
         (static_cast<uint32_t>(data[offset + 1]) << 8) |
         (static_cast<uint32_t>(data[offset + 2]) << 16) |
         (static_cast<uint32_t>(data[offset + 3]) << 24);
}

void appendU32(std::vector<uint8_t>& data, uint32_t value) {
  data.push_back(static_cast<uint8_t>(value & 0xff));
  data.push_back(static_cast<uint8_t>((value >> 8) & 0xff));
  data.push_back(static_cast<uint8_t>((value >> 16) & 0xff));
  data.push_back(static_cast<uint8_t>((value >> 24) & 0xff));
}

std::vector<uint8_t> encodePayload(Method method,
                                   const std::vector<uint8_t>& input) {
  switch (method) {
    case Method::kRle:
      return rleEncode(input);
    case Method::kHuffman:
      return huffmanEncode(input);
    case Method::kLz77:
      return lz77Encode(input);
    case Method::kBwt: {
      BwtResult bwt = bwtTransform(input);
      std::vector<uint8_t> mtf = mtfEncode(bwt.last);
      std::vector<uint8_t> rle = rleEncode(mtf);
      std::vector<uint8_t> huffman = huffmanEncode(rle);
      std::vector<uint8_t> payload;
      payload.reserve(huffman.size() + 4);
      appendU32(payload, static_cast<uint32_t>(bwt.primary));
      payload.insert(payload.end(), huffman.begin(), huffman.end());
      return payload;
    }
  }
  throw std::runtime_error("unknown compression method");
}

std::vector<uint8_t> decodePayload(Method method,
                                   const std::vector<uint8_t>& payload) {
  switch (method) {
    case Method::kRle:
      return rleDecode(payload);
    case Method::kHuffman:
      return huffmanDecode(payload);
    case Method::kLz77:
      return lz77Decode(payload);
    case Method::kBwt: {
      if (payload.size() < 4) throw std::runtime_error("corrupt BWT payload");
      uint32_t primary = readU32(payload, 0);
      std::vector<uint8_t> huffman(payload.begin() + 4, payload.end());
      std::vector<uint8_t> rle = huffmanDecode(huffman);
      std::vector<uint8_t> mtf = rleDecode(rle);
      std::vector<uint8_t> last = mtfDecode(mtf);  // undo move-to-front
      BwtResult bwt{last, static_cast<int>(primary)};
      return bwtInverse(bwt);
    }
  }
  throw std::runtime_error("unknown compression method");
}

}  // namespace

const char* methodName(Method method) {
  switch (method) {
    case Method::kRle: return "rle";
    case Method::kHuffman: return "huffman";
    case Method::kLz77: return "lz77";
    case Method::kBwt: return "bwt";
  }
  return "unknown";
}

std::vector<uint8_t> compress(Method method, const std::vector<uint8_t>& input) {
  std::vector<uint8_t> output;
  output.reserve(input.size() + 64);
  output.insert(output.end(), kMagic, kMagic + 4);
  output.push_back(static_cast<uint8_t>(method));
  std::vector<uint8_t> payload = encodePayload(method, input);
  output.insert(output.end(), payload.begin(), payload.end());
  return output;
}

std::vector<uint8_t> decompress(const std::vector<uint8_t>& input) {
  if (input.size() < 5 || input[0] != kMagic[0] || input[1] != kMagic[1] ||
      input[2] != kMagic[2] || input[3] != kMagic[3]) {
    throw std::runtime_error("not a Squeeze file (bad magic)");
  }
  Method method = static_cast<Method>(input[4]);
  std::vector<uint8_t> payload(input.begin() + 5, input.end());
  return decodePayload(method, payload);
}

}  // namespace squeeze
