#include "evaluator.hpp"

#include <algorithm>
#include <optional>
#include <string>

#include "analyzer.hpp"
#include "tokenizer.hpp"

namespace {

using DocIds = std::vector<std::size_t>;

using MaybeDocIds = std::optional<DocIds>;

DocIds doc_ids_of(const std::vector<Posting>& postings) {
    DocIds out;
    out.reserve(postings.size());
    for (const Posting& posting : postings) {
        out.push_back(posting.doc_id);
    }
    return out;
}

const Posting* find_posting(const std::vector<Posting>& postings, std::size_t doc_id) {
    const auto it = std::lower_bound(
        postings.begin(), postings.end(), doc_id,
        [](const Posting& posting, std::size_t target) { return posting.doc_id < target; });

    if (it == postings.end() || it->doc_id != doc_id) {
        return nullptr;
    }
    return &*it;
}

DocIds shift_positions(const std::vector<std::size_t>& positions, std::size_t offset) {
    DocIds out;
    out.reserve(positions.size());
    for (const std::size_t position : positions) {
        if (position >= offset) {
            out.push_back(position - offset);
        }
    }
    return out;
}

bool phrase_occurs_in(const std::vector<const std::vector<Posting>*>& lists,
                      const std::vector<std::size_t>& offsets,
                      std::size_t doc_id) {
    const Posting* first = find_posting(*lists.front(), doc_id);
    if (first == nullptr) {
        return false;
    }

    DocIds starts = first->positions;
    for (std::size_t i = 1; i < lists.size() && !starts.empty(); i++) {
        const Posting* posting = find_posting(*lists[i], doc_id);
        if (posting == nullptr) {
            return false;
        }
        starts = intersect(starts, shift_positions(posting->positions, offsets[i]));
    }

    return !starts.empty();
}

MaybeDocIds evaluate_terms_as_phrase(const std::vector<Token>& terms,
                                     const InvertedIndex& index) {
    std::vector<const std::vector<Posting>*> lists;
    std::vector<std::size_t> offsets;
    lists.reserve(terms.size());
    offsets.reserve(terms.size());

    for (const Token& term : terms) {
        const std::vector<Posting>& postings = index.postings(term.text);
        if (postings.empty()) {
            return DocIds{};
        }
        lists.push_back(&postings);
        offsets.push_back(term.position - terms.front().position);
    }

    DocIds candidates = doc_ids_of(*lists.front());
    for (std::size_t i = 1; i < lists.size() && !candidates.empty(); i++) {
        candidates = intersect(candidates, doc_ids_of(*lists[i]));
    }

    if (lists.size() == 1) {
        return candidates;
    }

    DocIds out;
    out.reserve(candidates.size());
    for (const std::size_t doc_id : candidates) {
        if (phrase_occurs_in(lists, offsets, doc_id)) {
            out.push_back(doc_id);
        }
    }
    return out;
}

MaybeDocIds evaluate_node(const QueryNode& node, const InvertedIndex& index);

MaybeDocIds evaluate_leaf(const QueryNode& node, const InvertedIndex& index) {
    const std::vector<Token> terms = analyze(node.text);
    if (terms.empty()) {
        return std::nullopt;
    }
    return evaluate_terms_as_phrase(terms, index);
}

std::vector<DocIds> evaluate_children(const QueryNode& node, const InvertedIndex& index) {
    std::vector<DocIds> results;
    results.reserve(node.children.size());

    for (const std::unique_ptr<QueryNode>& child : node.children) {
        MaybeDocIds result = evaluate_node(*child, index);
        if (result.has_value()) {
            results.push_back(std::move(*result));
        }
    }
    return results;
}

MaybeDocIds evaluate_and(const QueryNode& node, const InvertedIndex& index) {
    std::vector<DocIds> results = evaluate_children(node, index);
    if (results.empty()) {
        return std::nullopt;
    }

    std::sort(results.begin(), results.end(),
              [](const DocIds& a, const DocIds& b) { return a.size() < b.size(); });

    DocIds out = std::move(results.front());
    for (std::size_t i = 1; i < results.size() && !out.empty(); i++) {
        out = intersect(out, results[i]);
    }
    return out;
}

MaybeDocIds evaluate_or(const QueryNode& node, const InvertedIndex& index) {
    std::vector<DocIds> results = evaluate_children(node, index);
    if (results.empty()) {
        return std::nullopt;
    }

    DocIds out = std::move(results.front());
    for (std::size_t i = 1; i < results.size(); i++) {
        out = unite(out, results[i]);
    }
    return out;
}

MaybeDocIds evaluate_not(const QueryNode& node, const InvertedIndex& index) {
    MaybeDocIds operand = evaluate_node(*node.children.front(), index);
    if (!operand.has_value()) {
        return std::nullopt;
    }
    return subtract(all_documents(index), *operand);
}

MaybeDocIds evaluate_node(const QueryNode& node, const InvertedIndex& index) {
    switch (node.kind) {
    case QueryNodeKind::Term:
    case QueryNodeKind::Phrase:
        return evaluate_leaf(node, index);
    case QueryNodeKind::And:
        return evaluate_and(node, index);
    case QueryNodeKind::Or:
        return evaluate_or(node, index);
    case QueryNodeKind::Not:
        return evaluate_not(node, index);
    }
    return std::nullopt;
}

}

std::vector<std::size_t> intersect(const std::vector<std::size_t>& a,
                                   const std::vector<std::size_t>& b) {
    std::vector<std::size_t> out;
    out.reserve(std::min(a.size(), b.size()));

    std::size_t i = 0;
    std::size_t j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] < b[j]) {
            i++;
        } else if (b[j] < a[i]) {
            j++;
        } else {
            out.push_back(a[i]);
            i++;
            j++;
        }
    }
    return out;
}

std::vector<std::size_t> unite(const std::vector<std::size_t>& a,
                               const std::vector<std::size_t>& b) {
    std::vector<std::size_t> out;
    out.reserve(a.size() + b.size());

    std::size_t i = 0;
    std::size_t j = 0;
    while (i < a.size() && j < b.size()) {
        if (a[i] < b[j]) {
            out.push_back(a[i++]);
        } else if (b[j] < a[i]) {
            out.push_back(b[j++]);
        } else {
            out.push_back(a[i++]);
            j++;
        }
    }
    out.insert(out.end(), a.begin() + static_cast<std::ptrdiff_t>(i), a.end());
    out.insert(out.end(), b.begin() + static_cast<std::ptrdiff_t>(j), b.end());
    return out;
}

std::vector<std::size_t> subtract(const std::vector<std::size_t>& a,
                                  const std::vector<std::size_t>& b) {
    std::vector<std::size_t> out;
    out.reserve(a.size());

    std::size_t i = 0;
    std::size_t j = 0;
    while (i < a.size()) {
        if (j == b.size() || a[i] < b[j]) {
            out.push_back(a[i++]);
        } else if (b[j] < a[i]) {
            j++;
        } else {
            i++;
            j++;
        }
    }
    return out;
}

std::vector<std::size_t> all_documents(const InvertedIndex& index) {
    std::vector<std::size_t> out;
    out.reserve(index.document_count());
    for (std::size_t i = 0; i < index.document_count(); i++) {
        out.push_back(i);
    }
    return out;
}

std::vector<std::size_t> evaluate(const QueryNode& node, const InvertedIndex& index) {
    MaybeDocIds result = evaluate_node(node, index);
    return result.has_value() ? std::move(*result) : DocIds{};
}
