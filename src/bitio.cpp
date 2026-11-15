#include "bitio.h"

namespace squeeze {

void BitWriter::writeBit(int bit) {
  current_ = static_cast<uint8_t>((current_ << 1) | (bit & 1));
  ++bit_count_;
  if (bit_count_ == 8) {
    buffer_.push_back(current_);
    current_ = 0;
    bit_count_ = 0;
  }
}

void BitWriter::writeBits(uint32_t value, int count) {
  for (int i = count - 1; i >= 0; --i) {
    writeBit((value >> i) & 1);
  }
}

void BitWriter::writeByte(uint8_t byte) {
  writeBits(byte, 8);
}

std::vector<uint8_t> BitWriter::finish() {
  if (bit_count_ > 0) {
    current_ = static_cast<uint8_t>(current_ << (8 - bit_count_));
    buffer_.push_back(current_);
    current_ = 0;
    bit_count_ = 0;
  }
  return buffer_;
}

BitReader::BitReader(const std::vector<uint8_t>& data) : data_(data) {}

void BitReader::refill() {
  if (byte_pos_ < data_.size()) {
    current_ = data_[byte_pos_++];
    bit_count_ = 8;
  } else {
    current_ = 0;
    bit_count_ = 0;
  }
}

int BitReader::readBit() {
  if (bit_count_ == 0) refill();
  if (bit_count_ == 0) return -1;
  --bit_count_;
  return (current_ >> bit_count_) & 1;
}

uint32_t BitReader::readBits(int count) {
  uint32_t value = 0;
  for (int i = 0; i < count; ++i) {
    int bit = readBit();
    value = static_cast<uint32_t>((value << 1) | (bit > 0 ? 1 : 0));
  }
  return value;
}

uint8_t BitReader::readByte() {
  return static_cast<uint8_t>(readBits(8));
}

}  // namespace squeeze
