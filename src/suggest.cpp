#include "suggest.hpp"

#include <algorithm>
#include <utility>

#include "edit_distance.hpp"

void BkTree::insert(std::string term, std::size_t document_frequency) {
    if (!root_) {
        root_ = std::make_unique<Node>();
        root_->term = std::move(term);
        root_->document_frequency = document_frequency;
        size_ = 1;
        return;
    }

    Node* node = root_.get();
    while (true) {
        const std::size_t distance = edit_distance(term, node->term);
        if (distance == 0) {
            return;
        }

        std::unique_ptr<Node>& child = node->children[distance];
        if (!child) {
            child = std::make_unique<Node>();
            child->term = std::move(term);
            child->document_frequency = document_frequency;
            size_++;
            return;
        }
        node = child.get();
    }
}

std::vector<Suggestion> BkTree::search(std::string_view word, std::size_t max_distance,
                                       std::size_t limit) const {
    std::vector<Suggestion> found;
    if (!root_) {
        return found;
    }

    std::vector<const Node*> pending{root_.get()};
    while (!pending.empty()) {
        const Node* node = pending.back();
        pending.pop_back();

        const std::size_t distance = bounded_edit_distance(word, node->term, max_distance);
        if (distance <= max_distance) {
            found.push_back(Suggestion{node->term, distance, node->document_frequency});
        }

        const std::size_t exact = distance <= max_distance
                                      ? distance
                                      : edit_distance(word, node->term);
        const std::size_t low = exact > max_distance ? exact - max_distance : 0;
        const std::size_t high = exact + max_distance;

        for (const auto& [child_distance, child] : node->children) {
            if (child_distance >= low && child_distance <= high) {
                pending.push_back(child.get());
            }
        }
    }

    std::sort(found.begin(), found.end(), [](const Suggestion& a, const Suggestion& b) {
        if (a.distance != b.distance) {
            return a.distance < b.distance;
        }
        if (a.document_frequency != b.document_frequency) {
            return a.document_frequency > b.document_frequency;
        }
        return a.term < b.term;
    });

    if (found.size() > limit) {
        found.resize(limit);
    }
    return found;
}

BkTree build_bk_tree(const InvertedIndex& index) {
    BkTree tree;
    for (const std::string& term : index.terms()) {
        tree.insert(term, index.document_frequency(term));
    }
    return tree;
}
