#include "huffman.h"

#include <algorithm>
#include <cstddef>
#include <queue>
#include <stdexcept>
#include <utility>

#include "bitio.h"

namespace squeeze {
namespace {

struct Node {
  int64_t weight = 0;
  int symbol = -1;  // -1 for internal nodes
  Node* left = nullptr;
  Node* right = nullptr;
};

struct LengthEntry {
  uint8_t symbol = 0;
  int length = 0;
};

// Derives code lengths with a standard Huffman tree.
std::vector<int> codeLengths(const std::vector<uint8_t>& input) {
  std::vector<int64_t> frequencies(256, 0);
  for (uint8_t byte : input) ++frequencies[byte];

  auto compare = [](const Node* a, const Node* b) {
    return a->weight > b->weight;  // min-heap
  };
  std::priority_queue<Node*, std::vector<Node*>, decltype(compare)> heap(compare);
  std::vector<Node> nodes;
  nodes.reserve(512);
  for (int symbol = 0; symbol < 256; ++symbol) {
    if (frequencies[symbol] == 0) continue;
    nodes.push_back(Node{frequencies[symbol], symbol, nullptr, nullptr});
    heap.push(&nodes.back());
  }
  if (heap.size() == 1) {
    nodes.push_back(Node{0, -1, heap.top(), nullptr});
    heap.pop();
    heap.push(&nodes.back());
  }
  while (heap.size() > 1) {
    Node* first = heap.top(); heap.pop();
    Node* second = heap.top(); heap.pop();
    nodes.push_back(Node{first->weight + second->weight, -1, first, second});
    heap.push(&nodes.back());
  }
  Node* root = heap.empty() ? nullptr : heap.top();

  std::vector<int> lengths(256, 0);
  if (root == nullptr) return lengths;
  std::vector<std::pair<Node*, int>> stack;
  stack.emplace_back(root, 0);
  while (!stack.empty()) {
    Node* node = stack.back().first;
    int depth = stack.back().second;
    stack.pop_back();
    if (node->symbol >= 0) {
      lengths[node->symbol] = depth;
    } else {
      if (node->right) stack.emplace_back(node->right, depth + 1);
      if (node->left) stack.emplace_back(node->left, depth + 1);
    }
  }
  return lengths;
}

// Canonical Huffman codes: symbols sorted by (length, symbol), codes assigned
// sequentially so the decoder can rebuild the table from the lengths alone.
std::vector<uint32_t> canonicalCodes(const std::vector<int>& lengths,
                                     std::vector<int>& order,
                                     int& maxLength) {
  order.clear();
  for (int symbol = 0; symbol < 256; ++symbol) {
    if (lengths[symbol] > 0) order.push_back(symbol);
  }
  std::sort(order.begin(), order.end(), [&](int a, int b) {
    if (lengths[a] != lengths[b]) return lengths[a] < lengths[b];
    return a < b;
  });
  std::vector<uint32_t> codes(256, 0);
  maxLength = 0;
  uint32_t code = 0;
  int previousLength = 0;
  for (int symbol : order) {
    code <<= (lengths[symbol] - previousLength);
    codes[symbol] = code++;
    previousLength = lengths[symbol];
    maxLength = std::max(maxLength, lengths[symbol]);
  }
  return codes;
}

void writeHeader(const std::vector<int>& lengths, const std::vector<int>& order,
                 size_t symbolTotal, BitWriter& writer) {
  writer.writeBits(static_cast<uint32_t>(order.size()), 9);
  writer.writeBits(static_cast<uint32_t>(symbolTotal), 32);
  for (int symbol : order) {
    writer.writeByte(static_cast<uint8_t>(symbol));
    writer.writeBits(static_cast<uint32_t>(lengths[symbol]), 5);
  }
}

}  // namespace

std::vector<uint8_t> huffmanEncode(const std::vector<uint8_t>& input) {
  std::vector<int> lengths = codeLengths(input);
  if (input.empty()) return {};

  std::vector<int> order;
  int maxLength = 0;
  std::vector<uint32_t> codes = canonicalCodes(lengths, order, maxLength);

  BitWriter writer;
  writeHeader(lengths, order, input.size(), writer);
  for (uint8_t byte : input) {
    writer.writeBits(codes[byte], lengths[byte]);
  }
  return writer.finish();
}

std::vector<uint8_t> huffmanDecode(const std::vector<uint8_t>& input) {
  if (input.empty()) return {};
  BitReader reader(input);
  int symbolCount = static_cast<int>(reader.readBits(9));
  size_t symbolTotal = static_cast<size_t>(reader.readBits(32));
  std::vector<int> lengths(256, 0);
  std::vector<uint8_t> symbols;
  int maxLength = 0;
  for (int i = 0; i < symbolCount; ++i) {
    uint8_t symbol = reader.readByte();
    int length = static_cast<int>(reader.readBits(5));
    lengths[symbol] = length;
    symbols.push_back(symbol);
    maxLength = std::max(maxLength, length);
  }
  if (maxLength <= 0 || maxLength > 32) {
    throw std::runtime_error("corrupt Huffman header");
  }

  // Rebuild canonical codes.
  std::sort(symbols.begin(), symbols.end(), [&](uint8_t a, uint8_t b) {
    if (lengths[a] != lengths[b]) return lengths[a] < lengths[b];
    return a < b;
  });
  std::vector<uint32_t> codes(256, 0);
  {
    uint32_t code = 0;
    int previousLength = 0;
    for (uint8_t symbol : symbols) {
      code <<= (lengths[symbol] - previousLength);
      codes[symbol] = code++;
      previousLength = lengths[symbol];
    }
  }

  // Canonical decoding tables: firstCode[L] is the numerically smallest code
  // of length L, and table[L] lists the symbols of that length in code order.
  std::vector<int> countByLength(static_cast<size_t>(maxLength) + 2, 0);
  for (uint8_t symbol : symbols) ++countByLength[lengths[symbol]];
  std::vector<uint32_t> firstCode(static_cast<size_t>(maxLength) + 2, 0);
  {
    uint32_t code = 0;
    for (int len = 1; len <= maxLength; ++len) {
      code = (code + static_cast<uint32_t>(countByLength[len - 1])) << 1;
      firstCode[len] = code;
    }
  }
  std::vector<std::vector<uint8_t>> table(
      static_cast<size_t>(maxLength) + 2);
  for (uint8_t symbol : symbols) {
    table[static_cast<size_t>(lengths[symbol])].push_back(symbol);
  }

  std::vector<uint8_t> output;
  output.reserve(symbolTotal);
  uint32_t code = 0;
  int codeLength = 0;
  while (output.size() < symbolTotal) {
    int bit = reader.readBit();
    if (bit < 0) break;
    code = (code << 1) | static_cast<uint32_t>(bit);
    ++codeLength;
    if (codeLength <= maxLength && code >= firstCode[codeLength]) {
      uint32_t offset = code - firstCode[codeLength];
      if (offset < static_cast<uint32_t>(countByLength[codeLength])) {
        output.push_back(table[static_cast<size_t>(codeLength)]
                              [static_cast<size_t>(offset)]);
        code = 0;
        codeLength = 0;
      }
    }
  }
  return output;
}

}  // namespace squeeze
