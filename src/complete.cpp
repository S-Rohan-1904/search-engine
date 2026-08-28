#include "complete.hpp"

#include <algorithm>

void Trie::insert(std::string_view term, std::size_t document_frequency) {
    Node* node = &root_;
    for (const char c : term) {
        std::unique_ptr<Node>& child = node->children[c];
        if (!child) {
            child = std::make_unique<Node>();
        }
        node = child.get();
    }

    if (!node->terminal) {
        size_++;
    }
    node->terminal = true;
    node->document_frequency = document_frequency;
}

void Trie::collect(const Node& node, std::string& prefix, std::vector<Completion>& out) {
    if (node.terminal) {
        out.push_back(Completion{prefix, node.document_frequency});
    }

    std::vector<char> keys;
    keys.reserve(node.children.size());
    for (const auto& [c, child] : node.children) {
        keys.push_back(c);
    }
    std::sort(keys.begin(), keys.end());

    for (const char c : keys) {
        prefix.push_back(c);
        collect(*node.children.at(c), prefix, out);
        prefix.pop_back();
    }
}

std::vector<Completion> Trie::complete(std::string_view prefix, std::size_t limit) const {
    const Node* node = &root_;
    for (const char c : prefix) {
        const auto child = node->children.find(c);
        if (child == node->children.end()) {
            return {};
        }
        node = child->second.get();
    }

    std::vector<Completion> out;
    std::string path(prefix);
    collect(*node, path, out);

    std::sort(out.begin(), out.end(), [](const Completion& a, const Completion& b) {
        if (a.document_frequency != b.document_frequency) {
            return a.document_frequency > b.document_frequency;
        }
        return a.term < b.term;
    });

    if (out.size() > limit) {
        out.resize(limit);
    }
    return out;
}

Trie build_trie(const InvertedIndex& index) {
    Trie trie;
    for (const std::string& term : index.terms()) {
        trie.insert(term, index.document_frequency(term));
    }
    return trie;
}
