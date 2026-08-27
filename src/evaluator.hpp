#pragma once

#include <cstddef>
#include <vector>

#include "index.hpp"
#include "query.hpp"

// Set operations over sorted, duplicate-free document id lists.
//
// Postings lists come out of the index in exactly that shape, so these three
// functions are all a boolean engine needs. Each is a single linear pass over
// both inputs, which is only possible because the inputs are sorted: comparing
// the two front elements is enough to decide what to do next and which side to
// advance.
//
// The result is sorted and duplicate-free too, so operations compose.
std::vector<std::size_t> intersect(const std::vector<std::size_t>& a,
                                   const std::vector<std::size_t>& b);
std::vector<std::size_t> unite(const std::vector<std::size_t>& a,
                               const std::vector<std::size_t>& b);
std::vector<std::size_t> subtract(const std::vector<std::size_t>& a,
                                  const std::vector<std::size_t>& b);

// Every document id in an index, ascending. This is the universe NOT
// complements against.
std::vector<std::size_t> all_documents(const InvertedIndex& index);

// The documents matching a parsed query, ascending by id.
//
// Query terms are analyzed with the same pipeline the documents were, so
// "Cats" finds what was indexed as "cat".
//
// A term that analyzes to nothing carries no constraint rather than matching
// nothing. `the cat` therefore means `cat`, not the empty set, and `NOT the`
// excludes nothing. A query consisting only of such terms matches no documents,
// because there is nothing left to search for.
//
// A phrase matches a document when its terms occur consecutively there, with
// the same spacing they have in the phrase after analysis. Stopwords inside a
// phrase leave their gap behind, so "the tiny document" still requires one
// unmatched word between nothing and `tini`, which is how `a tiny document`
// matches it too.
std::vector<std::size_t> evaluate(const QueryNode& node, const InvertedIndex& index);
