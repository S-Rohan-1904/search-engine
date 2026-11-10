#include "suggest.hpp"

#include <algorithm>
#include <utility>

#include "analyzer.hpp"
#include "edit_distance.hpp"
#include "evaluator.hpp"
#include "query.hpp"

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

namespace {

struct Correction {
    std::string term;
    bool word_was_indexed;
};

std::optional<Correction> correct_word(const std::string& word, const InvertedIndex& index,
                                       const BkTree& tree) {
    constexpr std::size_t kRareRatio = 100;
    constexpr std::size_t kRareMinimumLength = 4;

    const std::vector<Token> analyzed = analyze(word);
    if (analyzed.size() != 1) {
        return std::nullopt;
    }

    const std::string& term = analyzed.front().text;
    const std::size_t frequency = index.document_frequency(term);
    if (frequency != 0 && term.size() < kRareMinimumLength) {
        return std::nullopt;
    }

    const std::size_t max_distance = term.size() <= 4 ? 1 : 2;
    for (const Suggestion& suggestion : tree.search(term, max_distance, 10)) {
        if (suggestion.distance == 0 || suggestion.document_frequency < kRareRatio * frequency) {
            continue;
        }
        const std::vector<Token> again = analyze(suggestion.term);
        if (again.size() == 1 && again.front().text == suggestion.term) {
            return Correction{suggestion.term, frequency != 0};
        }
    }
    return std::nullopt;
}

std::string spell(const QueryToken& token) {
    switch (token.kind) {
        case QueryTokenKind::Term:
            return token.text;
        case QueryTokenKind::Phrase:
            return "\"" + token.text + "\"";
        case QueryTokenKind::And:
            return "AND";
        case QueryTokenKind::Or:
            return "OR";
        case QueryTokenKind::Not:
            return "NOT";
        case QueryTokenKind::LParen:
            return "(";
        case QueryTokenKind::RParen:
            return ")";
    }
    return "";
}

}

std::optional<std::string> correct_query(std::string_view query, const InvertedIndex& index,
                                         const BkTree& tree) {
    std::vector<QueryToken> tokens;
    std::string error;
    if (!lex_query(query, tokens, error)) {
        return std::nullopt;
    }

    constexpr std::size_t kFewMatches = 5;
    std::optional<bool> few_matches;
    const auto finds_little = [&]() {
        if (!few_matches.has_value()) {
            const ParseResult parsed = parse_query(query);
            few_matches = !parsed.root || evaluate(*parsed.root, index).size() < kFewMatches;
        }
        return *few_matches;
    };

    bool changed = false;
    std::string corrected;
    for (QueryToken& token : tokens) {
        if (token.kind == QueryTokenKind::Term) {
            const std::optional<Correction> better = correct_word(token.text, index, tree);
            if (better.has_value() && (!better->word_was_indexed || finds_little())) {
                token.text = better->term;
                changed = true;
            }
        }

        const std::string text = spell(token);
        if (!corrected.empty() && corrected.back() != '(' && text != ")") {
            corrected.push_back(' ');
        }
        corrected += text;
    }

    if (!changed) {
        return std::nullopt;
    }
    return corrected;
}
