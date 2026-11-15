// Squeeze — unit tests over every codec and the full pipelines.
#include <cstdint>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "../src/pipeline.h"
#include "../src/bitio.h"
#include "../src/bwt.h"
#include "../src/huffman.h"
#include "../src/lz77.h"
#include "../src/mtf.h"
#include "../src/rle.h"

using squeeze::Method;
using squeeze::compress;
using squeeze::decompress;

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const std::string& message) {
  ++checks;
  if (!condition) {
    ++failures;
    std::cerr << "FAIL: " << message << "\n";
  }
}

std::vector<uint8_t> randomBytes(size_t size, uint32_t seed) {
  std::mt19937 rng(seed);
  std::vector<uint8_t> data(size);
  for (auto& byte : data) byte = static_cast<uint8_t>(rng() & 0xff);
  return data;
}

std::vector<uint8_t> textLike(size_t size, uint32_t seed) {
  std::mt19937 rng(seed);
  std::vector<uint8_t> data(size);
  for (auto& byte : data) byte = static_cast<uint8_t>('a' + (rng() % 16));
  return data;
}

std::vector<uint8_t> runs(size_t size) {
  std::vector<uint8_t> data(size);
  for (size_t i = 0; i < size; ++i) data[i] = static_cast<uint8_t>(i / 7);
  return data;
}

void roundTrip(Method method, const std::vector<uint8_t>& data,
               const std::string& label) {
  std::vector<uint8_t> encoded = compress(method, data);
  std::vector<uint8_t> decoded = decompress(encoded);
  check(decoded == data, label + " round-trip");
}

void roundTripAll(const std::vector<uint8_t>& data, const std::string& label) {
  roundTrip(Method::kRle, data, label + " [rle]");
  roundTrip(Method::kHuffman, data, label + " [huffman]");
  roundTrip(Method::kLz77, data, label + " [lz77]");
  roundTrip(Method::kBwt, data, label + " [bwt]");
}

void testBitIo() {
  squeeze::BitWriter writer;
  writer.writeBits(0b101, 3);
  writer.writeBit(1);
  writer.writeBits(0b0110, 4);  // exactly 8 bits total
  std::vector<uint8_t> bytes = writer.finish();
  check(bytes.size() == 1, "bit writer packs 8 bits into 1 byte");
  squeeze::BitReader reader(bytes);
  check(reader.readBits(3) == 0b101, "bit reader round-trips 3 bits");
  check(reader.readBit() == 1, "bit reader round-trips 1 bit");
  check(reader.readBits(4) == 0b0110, "bit reader round-trips 4 bits");
  check(reader.readBit() == -1, "bit reader reports end of stream");
}

void testRle() {
  std::vector<uint8_t> data = {1, 1, 1, 1, 1, 2, 3, 3, 4};
  std::vector<uint8_t> encoded = squeeze::rleEncode(data);
  check(encoded.size() < data.size(), "rle compresses runs");
  check(squeeze::rleDecode(encoded) == data, "rle round-trip");
  std::vector<uint8_t> longRun(300, 7);
  check(squeeze::rleDecode(squeeze::rleEncode(longRun)) == longRun,
        "rle splits runs longer than 128");
}

void testHuffman() {
  std::vector<uint8_t> skewed;
  for (int i = 0; i < 1000; ++i) skewed.push_back('a');
  for (int i = 0; i < 100; ++i) skewed.push_back('b');
  for (int i = 0; i < 10; ++i) skewed.push_back('c');
  std::vector<uint8_t> encoded = squeeze::huffmanEncode(skewed);
  check(encoded.size() < skewed.size() / 2, "huffman exploits skew");
  check(squeeze::huffmanDecode(encoded) == skewed, "huffman round-trip");
  check(squeeze::huffmanEncode({}).empty() && squeeze::huffmanDecode({}).empty(),
        "huffman handles empty input");
  std::vector<uint8_t> single(50, 42);
  check(squeeze::huffmanDecode(squeeze::huffmanEncode(single)) == single,
        "huffman handles a single symbol");
}

void testLz77() {
  // A small random block repeated many times — the textbook redundant file.
  std::mt19937 rng(7);
  std::vector<uint8_t> block(100);
  for (auto& byte : block) byte = static_cast<uint8_t>(rng() & 0xff);
  std::vector<uint8_t> repetitive;
  for (int i = 0; i < 200; ++i) {
    repetitive.insert(repetitive.end(), block.begin(), block.end());
  }
  std::vector<uint8_t> encoded = squeeze::lz77Encode(repetitive);
  check(encoded.size() < repetitive.size() / 4, "lz77 finds long matches");
  check(squeeze::lz77Decode(encoded) == repetitive, "lz77 round-trip");
}

void testMtf() {
  std::vector<uint8_t> data = {3, 3, 3, 0, 255, 0};
  check(squeeze::mtfDecode(squeeze::mtfEncode(data)) == data, "mtf round-trip");
}

void testBwt() {
  // The classic example: BWT("banana") = ("annb", primary for '$aa').
  std::vector<uint8_t> banana = {'b', 'a', 'n', 'a', 'n', 'a'};
  squeeze::BwtResult result = squeeze::bwtTransform(banana);
  std::vector<uint8_t> expected = {'a', 'n', 'n', 'b', 'a', 'a'};
  check(result.last == expected, "bwt of banana matches the classic result");
  check(squeeze::bwtInverse(result) == banana, "bwt inverse restores banana");
  for (size_t size : {size_t{0}, size_t{1}, size_t{2}, size_t{1000}, size_t{4096}}) {
    std::vector<uint8_t> data = randomBytes(size, static_cast<uint32_t>(size) + 3);
    check(squeeze::bwtInverse(squeeze::bwtTransform(data)) == data,
          "bwt round-trip (random " + std::to_string(size) + " bytes)");
  }
}

void testContainers() {
  std::vector<uint8_t> empty;
  check(decompress(compress(Method::kBwt, empty)) == empty,
        "empty input round-trips through bwt");
  bool threw = false;
  try {
    decompress({1, 2, 3, 4, 5});
  } catch (const std::exception&) {
    threw = true;
  }
  check(threw, "decompress rejects files with a bad magic");
}

}  // namespace

int main() {
  testBitIo();
  testRle();
  testHuffman();
  testLz77();
  testMtf();
  testBwt();
  testContainers();

  roundTripAll({}, "empty");
  roundTripAll({0}, "single zero byte");
  roundTripAll({0, 0, 0, 0, 0}, "all zeros");
  roundTripAll(randomBytes(64, 1), "random-64");
  roundTripAll(randomBytes(5000, 2), "random-5k");
  roundTripAll(textLike(9000, 3), "text-like");
  roundTripAll(runs(6000), "long runs");

  std::cout << checks << " checks, " << failures << " failures\n";
  return failures == 0 ? 0 : 1;
}
