#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "tokenizer.hpp"

// One entry in a term's postings list.
//
// frequency always equals positions.size() as the index is built here, and is
// kept as its own field because the two are separate streams in any real
// index: positions cost several times what frequencies do, so an engine that
// does not need phrase search stores frequencies alone.
struct Posting {
    std::size_t doc_id;                   // ordinal into InvertedIndex::document_ids()
    std::size_t frequency;                // occurrences of the term in that document
    std::vector<std::size_t> positions;   // ascending token positions of those occurrences
};

// Maps each term to the documents containing it.
//
// Documents get a numeric id from the order they are added, and the string
// ids live in one vector that the numeric id indexes. Postings therefore hold
// a size_t rather than a string, which keeps them small and makes the
// intersection work in Phase 3 cheap.
class InvertedIndex {
public:
    // Registers doc_id and files every distinct term in terms under it.
    // Repeats of a term within one document share one posting, raising its
    // frequency and appending to its positions. A term's postings list length
    // is therefore its document frequency, and each posting carries that
    // document's count and the token positions behind it.
    void add_document(std::string doc_id, const std::vector<Token>& terms,
                      std::uint64_t fingerprint = 0);

    std::size_t document_count() const;

    // Number of distinct terms in the dictionary.
    std::size_t term_count() const;

    // Total postings across every term. Grows with the corpus far faster than
    // term_count does, which is why Phase 5 compresses postings and not the
    // dictionary.
    std::size_t posting_count() const;

    // How many documents contain term. Zero if the term is not indexed. The
    // term must already be analyzed.
    std::size_t document_frequency(std::string_view term) const;

    // The documents containing term, ascending by doc_id and without repeats.
    // Returns an empty list for a term that is not indexed, so callers never
    // have to check for absence separately. The term must already be analyzed:
    // looking up "Cats" finds nothing, because what was indexed is "cat".
    const std::vector<Posting>& postings(std::string_view term) const;

    // How many term occurrences a document contributed, counting repeats.
    //
    // This is the length of the analyzed term stream, not the document's word
    // count: stopwords were already dropped before the index saw it. Ranking
    // needs a length that matches what was indexed, so that is the right one.
    std::size_t document_length(std::size_t doc_id) const;

    // Mean document length across the corpus, or 0 for an empty one. BM25
    // divides by this to decide whether a document is long or short relative
    // to its peers.
    double average_document_length() const;

    // A hash of the bytes the document was built from, or 0 when unknown.
    //
    // Incremental indexing compares this against the file on disk to decide
    // whether a document needs re-analyzing. It is the only thing the index
    // remembers about a document's source, since the text itself is not stored.
    std::uint64_t document_fingerprint(std::size_t doc_id) const;

    // The ordinal of a document by its string id, if it is in the index.
    std::optional<std::size_t> find_document(std::string_view doc_id) const;

    // Rebuilds the token stream a document was indexed from.
    //
    // Every posting records the positions a term occurred at, so pairing each
    // position with its term and sorting recovers the stream, minus the byte
    // offsets the index never stored. That is what makes an incremental update
    // possible without re-reading the file: a document that has not changed can
    // be carried into a new index from the old one's postings alone.
    std::vector<Token> document_terms(std::size_t doc_id) const;

    // String ids, in the order the documents were added.
    const std::vector<std::string>& document_ids() const;

    // Every indexed term, sorted ascending.
    std::vector<std::string> terms() const;

    // Appends another index's documents after this one's.
    //
    // The other index's ordinals are local to it, so every one of its postings
    // is shifted by this index's current document count. Callers must append in
    // the order the documents should ultimately have, which is what keeps the
    // merged postings lists ascending without a sort.
    void append(InvertedIndex other);

    // Rebuilds an index from parts that were decoded from a file.
    //
    // The caller owns the invariants the build loop would otherwise guarantee:
    // every postings list ascending by doc_id and free of repeats, every
    // doc_id within range, and one length per document. The loader checks all
    // of that before calling this.
    static InvertedIndex from_parts(
        std::vector<std::string> document_ids,
        std::vector<std::size_t> document_lengths,
        std::vector<std::uint64_t> document_fingerprints,
        std::unordered_map<std::string, std::vector<Posting>> postings);

private:
    std::vector<std::string> document_ids_;
    std::vector<std::size_t> document_lengths_;
    std::vector<std::uint64_t> document_fingerprints_;
    std::unordered_map<std::string, std::vector<Posting>> postings_;
};

// Builds an index over the named documents, in the order given. Documents that
// fail to parse are skipped, so a document's ordinal is its position among the
// ones that parsed, not among the ids passed in.
InvertedIndex build_index_from(const std::filesystem::path& corpus_dir,
                               const std::vector<std::string>& ids);

// Builds an index over every document in corpus_dir, in the sorted id order
// that list_document_ids returns.
InvertedIndex build_index(const std::filesystem::path& corpus_dir);

// What an incremental update changed.
struct IndexUpdateReport {
    std::size_t added = 0;
    std::size_t updated = 0;
    std::size_t removed = 0;
    std::size_t unchanged = 0;
};

// Rebuilds an index against a corpus, re-analyzing only what changed.
//
// A document whose file still hashes to what the index recorded is carried
// across from the previous index rather than read and analyzed again. One that
// is new or changed is analyzed; one that has disappeared from the corpus is
// dropped.
//
// The result is a complete index in sorted id order, identical to what
// build_index would produce, rather than an index with tombstones in it.
// Deleting by tombstone and compacting later is what a system with continuous
// writes has to do; a corpus that is re-scanned in one pass can just be rebuilt,
// and the saving that matters is skipping the analysis, not the bookkeeping.
InvertedIndex update_index(const InvertedIndex& previous,
                           const std::filesystem::path& corpus_dir, IndexUpdateReport& report);

// The fingerprint of a document file, or 0 if it cannot be read.
std::uint64_t fingerprint_document(const std::filesystem::path& corpus_dir,
                                   const std::string& id);

// The same index, built on several threads.
//
// The corpus is split into contiguous slices, one per thread, and each slice is
// indexed independently before the results are merged in slice order. The
// result is identical to build_index for any thread count, including the
// document ordinals and therefore the encoded bytes.
InvertedIndex build_index_parallel(const std::filesystem::path& corpus_dir,
                                   std::size_t threads);
