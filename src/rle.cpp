#include "rle.h"

namespace squeeze {

std::vector<uint8_t> rleEncode(const std::vector<uint8_t>& input) {
  std::vector<uint8_t> output;
  output.reserve(input.size());
  size_t position = 0;
  while (position < input.size()) {
    // Measure the current run.
    size_t run = 1;
    while (position + run < input.size() && run < 128 &&
           input[position + run] == input[position]) {
      ++run;
    }
    if (run >= 2) {
      output.push_back(static_cast<uint8_t>(257 - run));  // repeat command
      output.push_back(input[position]);
      position += run;
    } else {
      // Gather a literal block (up to 128 bytes) that contains no long runs.
      size_t start = position;
      size_t block = 0;
      while (position < input.size() && block < 128) {
        size_t ahead = 1;
        while (position + ahead < input.size() && ahead < 128 &&
               input[position + ahead] == input[position]) {
          ++ahead;
        }
        if (ahead >= 2 && block > 0) break;  // flush literals before the run
        if (ahead >= 2 && block == 0) break;  // let the run encoder take it
        ++position;
        ++block;
      }
      output.push_back(static_cast<uint8_t>(block - 1));
      for (size_t i = start; i < start + block; ++i) output.push_back(input[i]);
    }
  }
  return output;
}

std::vector<uint8_t> rleDecode(const std::vector<uint8_t>& input) {
  std::vector<uint8_t> output;
  size_t position = 0;
  while (position < input.size()) {
    uint8_t control = input[position++];
    if (control <= 127) {
      size_t count = static_cast<size_t>(control) + 1;
      for (size_t i = 0; i < count && position < input.size(); ++i) {
        output.push_back(input[position++]);
      }
    } else {
      if (position >= input.size()) break;
      uint8_t value = input[position++];
      size_t count = static_cast<size_t>(257 - control);
      for (size_t i = 0; i < count; ++i) output.push_back(value);
    }
  }
  return output;
}

}  // namespace squeeze
