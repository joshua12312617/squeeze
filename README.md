# Squeeze

**A data compression toolkit built from scratch in C++17.** Four classic
pipelines — RLE, Huffman coding, LZ77, and the full bzip2-style chain
(Burrows–Wheeler → move-to-front → RLE → Huffman) — implemented without any
third-party libraries, with a benchmark CLI and a 52-check test suite that
runs in CI on both Linux and Windows.

[![C++](https://img.shields.io/badge/C%2B%2B-17-blue)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![CI](https://github.com/joshua12312617/squeeze/actions/workflows/ci.yml/badge.svg)](https://github.com/joshua12312617/squeeze/actions)

---

## The four pipelines

| Method | Algorithms inside | Best at |
|---|---|---|
| `rle` | PackBits run-length encoding | long runs of identical bytes |
| `huffman` | Huffman tree → **canonical codes** (length-only header, O(1)-style lookup decode) | skewed symbol distributions |
| `lz77` | sliding window (4096 B) with **hash-chain match finding**, greedy parsing | repetitive text |
| `bwt` | **suffix array** (prefix doubling) → Burrows–Wheeler transform → **move-to-front** → RLE → Huffman | everything text-like |

`bwt` is the flagship: it reuses the standalone RLE and Huffman codecs and
composes them exactly the way bzip2 does — so the project demonstrates both
individual algorithms *and* how real compressors are assembled from them.

## Real benchmark numbers

Built with GCC 13 at `-O2` and run by the CLI's `benchmark` command.

**On Squeeze's own source code (31,338 bytes):**

| Method | Compressed | Ratio |
|---|---|---|
| rle | 31,267 B | 99.8% |
| huffman | 19,778 B | 63.1% |
| lz77 | 10,045 B | 32.1% |
| **bwt** | **9,280 B** | **29.6%** |

**On a repetitive 200 KB file:**

| Method | Compressed | Ratio |
|---|---|---|
| huffman | 131,827 B | 65.9% |
| lz77 | 7,247 B | 3.6% |
| **bwt** | **730 B** | **0.4%** |

Every row verifies a byte-exact round-trip before reporting.

## Build and run

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

./build/squeeze compress input.txt output.sqz --method bwt
./build/squeeze decompress output.sqz restored.txt
./build/squeeze benchmark input.txt
```

(On Windows with MSVC use `--config Release`; on MinGW the binary lands in
`build/` directly.)

## Usage

```text
squeeze compress <input> <output> --method {rle|huffman|lz77|bwt}
squeeze decompress <input> <output>      # format auto-detected
squeeze benchmark <file>                 # all four methods, sizes, timings
```

The container format is `SQZ1` + one method byte + the payload — self-describing,
so decompression never needs the method passed in.

## Architecture

```text
src/
├── bitio.h/.cpp       bit-packing reader/writer over byte buffers
├── rle.h/.cpp         PackBits RLE codec
├── huffman.h/.cpp     tree construction, canonical codes, table decode
├── lz77.h/.cpp        sliding window + hash-chain match finder
├── mtf.h/.cpp         move-to-front transform
├── bwt.h/.cpp         suffix array (prefix doubling), BWT and its inverse
│                      via LF-mapping over the rotation matrix
├── pipeline.h/.cpp    the SQZ1 container and composed codec chains
└── main.cpp           CLI: compress / decompress / benchmark
tests/test_main.cpp    52 checks incl. the classic BWT("banana") example
.github/workflows/ci.yml  builds + tests on Linux (gcc, -Werror) and Windows (MSVC)
```

Every module is independent — each codec works on plain byte vectors — which
is what makes the suite testable and the composition trivial.

## What the tests prove

- Byte-exact round-trips for all four methods across empty, tiny, random,
  text-like, and run-heavy inputs
- The classic reference case: `BWT("banana") = "annb.."` plus its inverse
- Canonical Huffman round-trips with a single-symbol alphabet
- LZ77 compresses a 200×-repeated block to under 25%
- The container rejects files with a bad magic
- Bit-level reader/writer edge cases including end-of-stream

## Notes and ideas

- The BWT suffix array is the simple O(n log² n) prefix-doubling construction;
  swapping in SA-IS would be the next algorithmic upgrade.
- LZ77 uses greedy parsing — lazy matching would buy a few more percent.
- An obvious extension: an entropy-coded LZ77 (a mini-DEFLATE).

## License

MIT — see [LICENSE](LICENSE).
