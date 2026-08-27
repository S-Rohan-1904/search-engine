#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "index.hpp"

// How integers are laid out in an encoded index.
//
// The three settings exist so the cost of each idea can be measured against
// the others rather than asserted. They read back identically; only the bytes
// differ.
enum class IndexEncoding {
    Plain,    // absolute values, eight bytes each
    Delta,    // gaps instead of absolute values, still eight bytes each
    VarByte,  // gaps, one to ten bytes each depending on magnitude
};

// Serializes an index into a self-describing byte buffer.
//
// The header records which encoding was used, so a reader never has to be told.
// Terms are written in sorted order, which makes the output byte-identical for
// a given index rather than dependent on hash iteration order.
std::vector<unsigned char> encode_index(const InvertedIndex& index, IndexEncoding encoding);

// Where an encoded index spends its bytes.
//
// The sections are the ones worth comparing: the dictionary grows roughly with
// the square root of the text while the postings grow linearly, so which half
// dominates tells you what is worth compressing.
struct IndexSizeReport {
    std::size_t total;
    std::size_t header;
    std::size_t documents;   // ids and lengths
    std::size_t dictionary;  // term strings and list lengths
    std::size_t postings;    // document ids and frequencies
    std::size_t positions;
};

// Encodes the index and reports the size of each section, without keeping the
// bytes.
IndexSizeReport measure_index(const InvertedIndex& index, IndexEncoding encoding);

// Rebuilds an index from a buffer produced by encode_index.
//
// Returns nullopt with `error` set on anything malformed: a bad magic number, an
// unknown version or encoding, a truncated buffer, a document id out of range,
// or a postings list that is not strictly ascending. A corrupt index must fail
// loudly rather than produce quietly wrong search results.
std::optional<InvertedIndex> decode_index(const std::vector<unsigned char>& bytes,
                                          std::string& error);

// File wrappers around the two functions above.
bool write_index_file(const InvertedIndex& index, const std::filesystem::path& path,
                      IndexEncoding encoding, std::string& error);
std::optional<InvertedIndex> read_index_file(const std::filesystem::path& path,
                                             std::string& error);

// Parses an encoding name: "plain", "delta" or "varbyte".
std::optional<IndexEncoding> parse_encoding(std::string_view name);
