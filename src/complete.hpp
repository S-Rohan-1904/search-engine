#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "index.hpp"

// A completion and how common it is.
struct Completion {
    std::string term;
    std::size_t document_frequency;
};

// A prefix tree over the index dictionary.
//
// Walking every term and testing starts_with is O(dictionary) per keystroke,
// which is the wrong shape for something that runs as fast as a person types. A
// trie walks the prefix once, in time proportional to the prefix rather than to
// the dictionary, and then collects only the subtree beneath it.
//
// Each node stores one character's worth of the path, so a prefix shared by
// many terms is stored once. That is the same idea front coding would use to
// compress the dictionary on disk.
class Trie {
public:
    void insert(std::string_view term, std::size_t document_frequency);

    // Terms beginning with `prefix`, most common first, then alphabetically.
    //
    // Ranking by document frequency is what makes a completion list useful:
    // the terms a searcher means are the ones the corpus talks about, not the
    // ones that happen to sort first.
    std::vector<Completion> complete(std::string_view prefix, std::size_t limit) const;

    std::size_t size() const { return size_; }

private:
    struct Node {
        std::unordered_map<char, std::unique_ptr<Node>> children;
        std::size_t document_frequency = 0;
        bool terminal = false;
    };

    static void collect(const Node& node, std::string& prefix, std::vector<Completion>& out);

    Node root_;
    std::size_t size_ = 0;
};

// Builds a trie over every term in an index.
Trie build_trie(const InvertedIndex& index);
