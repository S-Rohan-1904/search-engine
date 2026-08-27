#pragma once

#include <cstddef>
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
    void add_document(std::string doc_id, const std::vector<Token>& terms);

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

    // String ids, in the order the documents were added.
    const std::vector<std::string>& document_ids() const;

    // Every indexed term, sorted ascending.
    std::vector<std::string> terms() const;

private:
    std::vector<std::string> document_ids_;
    std::unordered_map<std::string, std::vector<Posting>> postings_;
};

// Builds an index over every document in corpus_dir, added in the sorted id
// order that list_document_ids returns. Documents that fail to parse are
// skipped.
InvertedIndex build_index(const std::filesystem::path& corpus_dir);
