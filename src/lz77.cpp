#include "lz77.h"

#include <algorithm>
#include <vector>

#include "bitio.h"

namespace squeeze {
namespace {

constexpr int kWindowSize = 4096;
constexpr int kMinMatch = 3;
constexpr int kMaxMatch = 66;  // kMinMatch + 63, fits the 6-bit length field
constexpr int kHashBits = 14;
constexpr size_t kHashSize = size_t{1} << kHashBits;

uint32_t hash3(uint8_t a, uint8_t b, uint8_t c) {
  return (static_cast<uint32_t>(a) << 10) ^
         (static_cast<uint32_t>(b) << 5) ^ static_cast<uint32_t>(c);
}

}  // namespace

std::vector<uint8_t> lz77Encode(const std::vector<uint8_t>& input) {
  BitWriter writer;
  writer.writeBits(static_cast<uint32_t>(input.size()), 32);

  // Hash chains: head[hash] -> newest position with that hash,
  // prev[pos] -> next older position in the same chain.
  std::vector<int> head(kHashSize, -1);
  std::vector<int> prev(input.size(), -1);

  size_t position = 0;
  while (position < input.size()) {
    int bestDistance = 0;
    int bestLength = 0;
    if (position + kMinMatch <= input.size()) {
      uint32_t hash = hash3(input[position], input[position + 1],
                            input[position + 2]) & (kHashSize - 1);
      int candidate = head[hash];
      int chainLength = 0;
      while (candidate >= 0 && chainLength < 64) {
        int distance = static_cast<int>(position) - candidate;
        if (distance <= 0 || distance > kWindowSize) break;
        int length = 0;
        int maxProbe = std::min(kMaxMatch,
                                static_cast<int>(input.size() - position));
        while (length < maxProbe &&
               input[candidate + length] == input[position + length]) {
          ++length;
        }
        if (length > bestLength) {
          bestLength = length;
          bestDistance = distance;
          if (length == kMaxMatch) break;
        }
        candidate = prev[static_cast<size_t>(candidate)];
        ++chainLength;
      }
    }
    if (bestLength >= kMinMatch) {
      writer.writeBit(1);
      writer.writeBits(static_cast<uint32_t>(bestDistance - 1), 12);
      writer.writeBits(static_cast<uint32_t>(bestLength - kMinMatch), 6);
    } else {
      writer.writeBit(0);
      writer.writeByte(input[position]);
      bestLength = 1;
    }
    for (int step = 0; step < bestLength; ++step) {
      if (position + kMinMatch - 1 < input.size()) {
        uint32_t hash = hash3(input[position], input[position + 1],
                              input[position + 2]) & (kHashSize - 1);
        prev[position] = head[hash];
        head[hash] = static_cast<int>(position);
      }
      ++position;
    }
  }
  return writer.finish();
}

std::vector<uint8_t> lz77Decode(const std::vector<uint8_t>& input) {
  BitReader reader(input);
  uint32_t size = reader.readBits(32);
  std::vector<uint8_t> output;
  output.reserve(size);
  while (output.size() < size) {
    int bit = reader.readBit();
    if (bit < 0) break;
    if (bit == 1) {
      int distance = static_cast<int>(reader.readBits(12)) + 1;
      int length = static_cast<int>(reader.readBits(6)) + kMinMatch;
      for (int i = 0; i < length && output.size() < size; ++i) {
        output.push_back(output[output.size() - static_cast<size_t>(distance)]);
      }
    } else {
      output.push_back(reader.readByte());
    }
  }
  return output;
}

}  // namespace squeeze
