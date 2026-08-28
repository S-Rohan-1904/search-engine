#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "index.hpp"

// A term and how far it is from the word that was asked about.
struct Suggestion {
    std::string term;
    std::size_t distance;
    std::size_t document_frequency;
};

// A metric tree over edit distance, for finding terms close to a misspelling.
//
// Scanning the whole dictionary and measuring every term works and is what a
// small corpus can afford. A BK-tree makes it sublinear by exploiting the
// triangle inequality: if the query is distance d from a node, any term within
// k of the query is between d - k and d + k of that node, so every child
// outside that band can be skipped without measuring anything inside it.
//
// The tree is built once per index and queried per word.
class BkTree {
public:
    void insert(std::string term, std::size_t document_frequency);

    // Terms within `max_distance` of `word`, nearest first.
    //
    // Ties are broken by document frequency, then alphabetically: among equally
    // close corrections, the one more of the corpus uses is the better guess,
    // and the alphabetical fallback keeps the answer stable.
    std::vector<Suggestion> search(std::string_view word, std::size_t max_distance,
                                   std::size_t limit) const;

    std::size_t size() const { return size_; }

private:
    struct Node {
        std::string term;
        std::size_t document_frequency;
        std::unordered_map<std::size_t, std::unique_ptr<Node>> children;
    };

    std::unique_ptr<Node> root_;
    std::size_t size_ = 0;
};

// Builds a BK-tree over every term in an index.
BkTree build_bk_tree(const InvertedIndex& index);
