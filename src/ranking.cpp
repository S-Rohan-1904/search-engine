#include "ranking.hpp"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>

#include "analyzer.hpp"
#include "tokenizer.hpp"

namespace {

constexpr double kBm25K1 = 1.2;
constexpr double kBm25B = 0.75;

bool ranks_before(const ScoredDocument& a, const ScoredDocument& b) {
    if (a.score != b.score) {
        return a.score > b.score;
    }
    return a.doc_id < b.doc_id;
}

std::vector<ScoredDocument> to_ranking(const std::unordered_map<std::size_t, double>& totals) {
    std::vector<ScoredDocument> out;
    out.reserve(totals.size());
    for (const auto& [doc_id, score] : totals) {
        out.push_back(ScoredDocument{doc_id, score});
    }
    std::sort(out.begin(), out.end(), ranks_before);
    return out;
}

}

std::vector<std::string> query_terms(const std::vector<std::string>& words) {
    std::vector<std::string> terms;
    for (const std::string& word : words) {
        for (const Token& token : analyze(word)) {
            terms.push_back(token.text);
        }
    }
    return terms;
}

std::vector<ScoredDocument> score_tfidf(const std::vector<std::string>& terms,
                                        const InvertedIndex& index) {
    const double document_count = static_cast<double>(index.document_count());
    std::unordered_map<std::size_t, double> totals;

    for (const std::string& term : terms) {
        const std::vector<Posting>& postings = index.postings(term);
        if (postings.empty()) {
            continue;
        }

        const double idf = std::log(document_count / static_cast<double>(postings.size()));
        for (const Posting& posting : postings) {
            const double weight = 1.0 + std::log(static_cast<double>(posting.frequency));
            totals[posting.doc_id] += weight * idf;
        }
    }

    return to_ranking(totals);
}

std::vector<ScoredDocument> score_bm25(const std::vector<std::string>& terms,
                                       const InvertedIndex& index) {
    const double document_count = static_cast<double>(index.document_count());
    const double average_length = index.average_document_length();
    std::unordered_map<std::size_t, double> totals;

    for (const std::string& term : terms) {
        const std::vector<Posting>& postings = index.postings(term);
        if (postings.empty()) {
            continue;
        }

        const double df = static_cast<double>(postings.size());
        const double idf = std::log(1.0 + (document_count - df + 0.5) / (df + 0.5));

        for (const Posting& posting : postings) {
            const double frequency = static_cast<double>(posting.frequency);
            const double length = static_cast<double>(index.document_length(posting.doc_id));
            const double normalized = average_length > 0.0 ? length / average_length : 1.0;
            const double denominator =
                frequency + kBm25K1 * (1.0 - kBm25B + kBm25B * normalized);

            totals[posting.doc_id] += idf * frequency * (kBm25K1 + 1.0) / denominator;
        }
    }

    return to_ranking(totals);
}

std::vector<ScoredDocument> top_k(const std::vector<ScoredDocument>& scored, std::size_t k) {
    if (k == 0) {
        return {};
    }

    std::priority_queue<ScoredDocument, std::vector<ScoredDocument>, decltype(&ranks_before)>
        heap(&ranks_before);

    for (const ScoredDocument& candidate : scored) {
        if (heap.size() < k) {
            heap.push(candidate);
        } else if (ranks_before(candidate, heap.top())) {
            heap.pop();
            heap.push(candidate);
        }
    }

    std::vector<ScoredDocument> out;
    out.reserve(heap.size());
    while (!heap.empty()) {
        out.push_back(heap.top());
        heap.pop();
    }

    std::reverse(out.begin(), out.end());
    return out;
}
