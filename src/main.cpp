// Squeeze — command-line interface: compress, decompress, benchmark.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

#include "pipeline.h"

namespace {

using squeeze::Method;
using squeeze::compress;
using squeeze::decompress;
using squeeze::methodName;

std::vector<uint8_t> readFile(const std::string& path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) throw std::runtime_error("cannot open file: " + path);
  return std::vector<uint8_t>(std::istreambuf_iterator<char>(stream),
                              std::istreambuf_iterator<char>());
}

void writeFile(const std::string& path, const std::vector<uint8_t>& data) {
  std::ofstream stream(path, std::ios::binary);
  if (!stream) throw std::runtime_error("cannot write file: " + path);
  stream.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(data.size()));
}

Method parseMethod(const std::string& name) {
  if (name == "rle") return Method::kRle;
  if (name == "huffman") return Method::kHuffman;
  if (name == "lz77") return Method::kLz77;
  if (name == "bwt") return Method::kBwt;
  throw std::runtime_error("unknown method '" + name +
                           "' (use rle, huffman, lz77, or bwt)");
}

void printUsage() {
  std::cout
      << "Squeeze — a data compression toolkit\n\n"
      << "Usage:\n"
      << "  squeeze compress <input> <output> --method {rle|huffman|lz77|bwt}\n"
      << "  squeeze decompress <input> <output>\n"
      << "  squeeze benchmark <file>\n";
}

int runCompress(const std::vector<std::string>& args) {
  if (args.size() < 5) throw std::runtime_error("usage: compress IN OUT --method M");
  Method method = parseMethod(args[4]);
  std::vector<uint8_t> input = readFile(args[2]);
  std::vector<uint8_t> output = compress(method, input);
  writeFile(args[3], output);
  std::cout << "compressed " << input.size() << " -> " << output.size()
            << " bytes (" << methodName(method) << ", "
            << std::fixed << std::setprecision(1)
            << (100.0 * output.size() / std::max<size_t>(input.size(), 1)) << "%)\n";
  return 0;
}

int runDecompress(const std::vector<std::string>& args) {
  if (args.size() < 4) throw std::runtime_error("usage: decompress IN OUT");
  std::vector<uint8_t> input = readFile(args[2]);
  std::vector<uint8_t> output = decompress(input);
  writeFile(args[3], output);
  std::cout << "decompressed " << input.size() << " -> " << output.size() << " bytes\n";
  return 0;
}

int runBenchmark(const std::vector<std::string>& args) {
  if (args.size() < 3) throw std::runtime_error("usage: benchmark FILE");
  std::vector<uint8_t> input = readFile(args[2]);
  std::cout << "input: " << input.size() << " bytes\n\n";
  std::cout << std::left << std::setw(12) << "method" << std::right
            << std::setw(12) << "size" << std::setw(10) << "ratio"
            << std::setw(10) << "ms" << "\n";
  std::cout << std::string(44, '-') << "\n";
  const Method methods[4] = {Method::kRle, Method::kHuffman, Method::kLz77,
                             Method::kBwt};
  for (Method method : methods) {
    auto start = std::chrono::steady_clock::now();
    std::vector<uint8_t> output = compress(method, input);
    auto stop = std::chrono::steady_clock::now();
    double milliseconds =
        std::chrono::duration<double, std::milli>(stop - start).count();

    auto decodeStart = std::chrono::steady_clock::now();
    std::vector<uint8_t> restored = decompress(output);
    auto decodeStop = std::chrono::steady_clock::now();
    double decodeMs =
        std::chrono::duration<double, std::milli>(decodeStop - decodeStart).count();
    bool ok = (restored == input);

    std::cout << std::left << std::setw(12) << methodName(method) << std::right
              << std::setw(12) << output.size() << std::setw(9)
              << std::fixed << std::setprecision(1)
              << (100.0 * output.size() / std::max<size_t>(input.size(), 1))
              << "%" << std::setw(10) << std::fixed << std::setprecision(1)
              << milliseconds << "ms"
              << "  (decode " << decodeMs << "ms, "
              << (ok ? "round-trip OK" : "ROUND-TRIP FAILED") << ")\n";
  }
  return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
  try {
    std::vector<std::string> args(argv, argv + argc);
    if (args.size() < 2) {
      printUsage();
      return 1;
    }
    if (args[1] == "compress") return runCompress(args);
    if (args[1] == "decompress") return runDecompress(args);
    if (args[1] == "benchmark") return runBenchmark(args);
    printUsage();
    return 1;
  } catch (const std::exception& error) {
    std::cerr << "squeeze: " << error.what() << "\n";
    return 2;
  }
}
