// Squeeze — bit-level I/O over byte buffers.
#pragma once

#include <cstdint>
#include <vector>

namespace squeeze {

// Writes bits least-significant-first into an internal byte buffer.
class BitWriter {
 public:
  void writeBit(int bit);
  void writeBits(uint32_t value, int count);  // 0 <= count <= 32
  void writeByte(uint8_t byte);
  // Flushes any partial byte and returns the finished buffer.
  std::vector<uint8_t> finish();

 private:
  std::vector<uint8_t> buffer_;
  uint8_t current_ = 0;
  int bit_count_ = 0;
};

// Reads bits least-significant-first from a byte buffer.
class BitReader {
 public:
  explicit BitReader(const std::vector<uint8_t>& data);
  int readBit();                 // 0/1, or -1 at end of stream
  uint32_t readBits(int count);  // 0 <= count <= 32
  uint8_t readByte();            // reads 8 bits; undefined past the end

 private:
  const std::vector<uint8_t>& data_;
  size_t byte_pos_ = 0;
  uint8_t current_ = 0;
  int bit_count_ = 0;
  void refill();
};

}  // namespace squeeze
