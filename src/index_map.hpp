#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "index.hpp"
#include "index_io.hpp"
#include "mapped_file.hpp"

// An index file that is read where it lies, instead of being rebuilt in memory.
//
// Version 2 wrote every term and posting back to back, so opening a file meant
// decoding all of it: on a 173 MB index that is 25 million postings, each one
// allocating its own positions vector, and it took over three seconds before
// the first query could run. Nothing about a query needs that. A query touches
// a handful of terms and the lengths of the documents it scores.
//
// Version 3 adds what random access needs: fixed-width tables for the per
// document arrays, and a table of offsets to the term entries, sorted by term.
// Opening the file is then a mmap and a header read; finding a term is a binary
// search over mapped bytes; and a postings list is decoded only when a query
// asks for that term.
//
// The payload encoding is unchanged, so the compression the earlier stages
// measured still applies.
class MappedIndex {
public:
    // Maps an index file. Returns nullopt with `error` set when the file is
    // missing, is not an index, or is not version 3 -- an older file is still
    // readable, just not this way, so the caller falls back to decoding it.
    static std::optional<MappedIndex> open(const std::filesystem::path& path, std::string& error);

    std::size_t document_count() const { return document_count_; }
    std::size_t term_count() const { return term_count_; }
    std::size_t posting_count() const { return posting_count_; }

    std::size_t document_length(std::size_t doc_id) const;
    std::uint64_t document_fingerprint(std::size_t doc_id) const;
    std::string_view document_id(std::size_t doc_id) const;

    // The postings for a term, decoded on the spot, or an empty list if the
    // term is absent. Not cached here: the index above owns that decision.
    std::vector<Posting> postings(std::string_view term) const;

    // The token stream a document was indexed from, rebuilt by walking every
    // term entry. Incremental update needs this to carry an unchanged document
    // across without re-reading it. It is proportional to the whole dictionary,
    // which is what makes update expensive on a large corpus.
    std::vector<Token> document_terms(std::size_t doc_id) const;

    // Every term, in the sorted order the file stores them in.
    std::vector<std::string> terms() const;

    // The term at table position `slot`, and its postings. Terms are stored in
    // sorted order, which is what lets an external merge walk several files at
    // once without holding any of them in memory.
    std::string_view term_at(std::size_t slot) const;
    std::vector<Posting> postings_at(std::size_t slot) const;

    // The total of every document length, for the BM25 average.
    std::uint64_t total_document_length() const { return total_document_length_; }

private:
    MappedFile data_;
    IndexEncoding encoding_ = IndexEncoding::VarByte;
    std::size_t document_count_ = 0;
    std::size_t term_count_ = 0;
    std::size_t posting_count_ = 0;
    std::size_t lengths_at_ = 0;
    std::size_t fingerprints_at_ = 0;
    std::size_t id_offsets_at_ = 0;
    std::size_t term_offsets_at_ = 0;
    std::uint64_t total_document_length_ = 0;
};

// Encodes an index in version 3, with the tables that make it mappable.
std::vector<unsigned char> encode_index_v3(const InvertedIndex& index, IndexEncoding encoding,
                                           IndexSizeReport* report);
