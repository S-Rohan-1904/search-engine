#include "index.hpp"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>

#include "analyzer.hpp"
#include "corpus.hpp"
#include "document.hpp"

void InvertedIndex::add_document(std::string doc_id, const std::vector<Token>& terms) {
    const std::size_t ordinal = document_ids_.size();
    document_ids_.push_back(std::move(doc_id));
    document_lengths_.push_back(terms.size());

    for (const Token& token : terms) {
        std::vector<Posting>& postings = postings_[token.text];
        if (postings.empty() || postings.back().doc_id != ordinal) {
            postings.push_back(Posting{ordinal, 1, {token.position}});
        } else {
            postings.back().frequency++;
            postings.back().positions.push_back(token.position);
        }
    }
}

std::size_t InvertedIndex::document_count() const {
    return document_ids_.size();
}

std::size_t InvertedIndex::term_count() const {
    return postings_.size();
}

std::size_t InvertedIndex::posting_count() const {
    std::size_t total = 0;
    for (const auto& [term, postings] : postings_) {
        total += postings.size();
    }
    return total;
}

std::size_t InvertedIndex::document_frequency(std::string_view term) const {
    return postings(term).size();
}

const std::vector<Posting>& InvertedIndex::postings(std::string_view term) const {
    static const std::vector<Posting> kEmpty;

    const auto it = postings_.find(std::string(term));
    return it == postings_.end() ? kEmpty : it->second;
}

std::size_t InvertedIndex::document_length(std::size_t doc_id) const {
    return doc_id < document_lengths_.size() ? document_lengths_[doc_id] : 0;
}

double InvertedIndex::average_document_length() const {
    if (document_lengths_.empty()) {
        return 0.0;
    }

    std::size_t total = 0;
    for (const std::size_t length : document_lengths_) {
        total += length;
    }
    return static_cast<double>(total) / static_cast<double>(document_lengths_.size());
}

const std::vector<std::string>& InvertedIndex::document_ids() const {
    return document_ids_;
}

std::vector<std::string> InvertedIndex::terms() const {
    std::vector<std::string> out;
    out.reserve(postings_.size());
    for (const auto& [term, postings] : postings_) {
        out.push_back(term);
    }
    std::sort(out.begin(), out.end());
    return out;
}

InvertedIndex InvertedIndex::from_parts(
    std::vector<std::string> document_ids,
    std::vector<std::size_t> document_lengths,
    std::unordered_map<std::string, std::vector<Posting>> postings) {
    InvertedIndex index;
    index.document_ids_ = std::move(document_ids);
    index.document_lengths_ = std::move(document_lengths);
    index.postings_ = std::move(postings);
    return index;
}

InvertedIndex build_index(const std::filesystem::path& corpus_dir) {
    InvertedIndex index;
    for (const std::string& id : list_document_ids(corpus_dir)) {
        const std::optional<Document> doc = read_document(corpus_dir, id);
        if (doc.has_value()) {
            index.add_document(id, analyze_document(*doc));
        }
    }
    return index;
}
