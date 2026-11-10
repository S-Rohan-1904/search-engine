#include "ranking.hpp"

#include <algorithm>
#include <cmath>
#include <future>
#include <queue>
#include <unordered_map>

#include "analyzer.hpp"
#include "thread_pool.hpp"
#include "tokenizer.hpp"

namespace {

constexpr double kBm25K1 = 1.2;
constexpr double kBm25B = 0.75;
constexpr double kBm25FTitleWeight = 2.0;

bool ranks_before(const ScoredDocument& a, const ScoredDocument& b) {
    if (a.score != b.score) {
        return a.score > b.score;
    }
    return a.doc_id < b.doc_id;
}

double tfidf_weight(const Posting& posting, double idf, const InvertedIndex&) {
    return (1.0 + std::log(static_cast<double>(posting.frequency))) * idf;
}

double bm25_weight(const Posting& posting, double idf, const InvertedIndex& index) {
    const double frequency = static_cast<double>(posting.frequency);
    const double average_length = index.average_document_length();
    const double length = static_cast<double>(index.document_length(posting.doc_id));
    const double normalized = average_length > 0.0 ? length / average_length : 1.0;
    const double denominator = frequency + kBm25K1 * (1.0 - kBm25B + kBm25B * normalized);
    return idf * frequency * (kBm25K1 + 1.0) / denominator;
}

double bm25f_weight(const Posting& posting, double idf, const InvertedIndex& index) {
    const TitleField title = index.document_title(posting.doc_id);
    const auto body_start = std::lower_bound(posting.positions.begin(), posting.positions.end(),
                                             title.span);
    const double title_frequency =
        static_cast<double>(std::distance(posting.positions.begin(), body_start));
    const double body_frequency = static_cast<double>(posting.frequency) - title_frequency;

    const double title_length = static_cast<double>(title.terms);
    const double body_length =
        static_cast<double>(index.document_length(posting.doc_id)) - title_length;
    const double average_title = index.average_title_length();
    const double average_body = index.average_document_length() - average_title;

    const auto normalized = [](double frequency, double length, double average) {
        if (frequency <= 0.0) {
            return 0.0;
        }
        const double ratio = average > 0.0 ? length / average : 1.0;
        return frequency / (1.0 - kBm25B + kBm25B * ratio);
    };

    const double combined = kBm25FTitleWeight * normalized(title_frequency, title_length,
                                                           average_title) +
                            normalized(body_frequency, body_length, average_body);
    return idf * combined / (kBm25K1 + combined);
}

double tfidf_idf(double document_count, double df) {
    return std::log(document_count / df);
}

double bm25_idf(double document_count, double df) {
    return std::log(1.0 + (document_count - df + 0.5) / (df + 0.5));
}

std::vector<ScoredDocument> score_shard(const std::vector<std::string>& terms,
                                        const InvertedIndex& index, Scorer scorer,
                                        std::size_t first_doc, std::size_t last_doc) {
    const double document_count = static_cast<double>(index.document_count());
    std::unordered_map<std::size_t, double> totals;

    for (const std::string& term : terms) {
        const std::vector<Posting>& postings = index.postings(term);
        if (postings.empty()) {
            continue;
        }

        const double df = static_cast<double>(postings.size());
        const double idf = scorer == Scorer::TfIdf ? tfidf_idf(document_count, df)
                                                   : bm25_idf(document_count, df);

        auto it = std::lower_bound(
            postings.begin(), postings.end(), first_doc,
            [](const Posting& posting, std::size_t target) { return posting.doc_id < target; });

        for (; it != postings.end() && it->doc_id < last_doc; ++it) {
            switch (scorer) {
                case Scorer::TfIdf:
                    totals[it->doc_id] += tfidf_weight(*it, idf, index);
                    break;
                case Scorer::Bm25:
                    totals[it->doc_id] += bm25_weight(*it, idf, index);
                    break;
                case Scorer::Bm25F:
                    totals[it->doc_id] += bm25f_weight(*it, idf, index);
                    break;
            }
        }
    }

    std::vector<ScoredDocument> out;
    out.reserve(totals.size());
    for (const auto& [doc_id, total] : totals) {
        out.push_back(ScoredDocument{doc_id, total});
    }
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

std::vector<ScoredDocument> score(const std::vector<std::string>& terms,
                                  const InvertedIndex& index, Scorer scorer,
                                  std::size_t threads) {
    std::vector<ScoredDocument> merged = score_unordered(terms, index, scorer, threads);
    std::sort(merged.begin(), merged.end(), ranks_before);
    return merged;
}

std::vector<ScoredDocument> score_unordered(const std::vector<std::string>& terms,
                                            const InvertedIndex& index, Scorer scorer,
                                            std::size_t threads) {
    const std::size_t documents = index.document_count();
    const std::size_t shard_count = std::max<std::size_t>(1, std::min(threads, documents));

    std::vector<ScoredDocument> merged;

    if (shard_count <= 1) {
        merged = score_shard(terms, index, scorer, 0, documents);
    } else {
        const std::size_t shard_size = (documents + shard_count - 1) / shard_count;

        ThreadPool pool(shard_count);
        std::vector<std::future<std::vector<ScoredDocument>>> pending;
        pending.reserve(shard_count);

        for (std::size_t first = 0; first < documents; first += shard_size) {
            const std::size_t last = std::min(first + shard_size, documents);
            pending.push_back(pool.submit([&terms, &index, scorer, first, last] {
                return score_shard(terms, index, scorer, first, last);
            }));
        }

        for (std::future<std::vector<ScoredDocument>>& future : pending) {
            const std::vector<ScoredDocument> shard = future.get();
            merged.insert(merged.end(), shard.begin(), shard.end());
        }
    }

    return merged;
}

std::optional<Scorer> parse_scorer(std::string_view name) {
    if (name == "bm25") {
        return Scorer::Bm25;
    }
    if (name == "bm25f") {
        return Scorer::Bm25F;
    }
    if (name == "tfidf") {
        return Scorer::TfIdf;
    }
    return std::nullopt;
}

const char* scorer_name(Scorer scorer) {
    switch (scorer) {
        case Scorer::TfIdf:
            return "tfidf";
        case Scorer::Bm25:
            return "bm25";
        case Scorer::Bm25F:
            return "bm25f";
    }
    return "bm25";
}

std::vector<ScoredDocument> score_tfidf(const std::vector<std::string>& terms,
                                        const InvertedIndex& index) {
    return score(terms, index, Scorer::TfIdf, 1);
}

std::vector<ScoredDocument> score_bm25(const std::vector<std::string>& terms,
                                       const InvertedIndex& index) {
    return score(terms, index, Scorer::Bm25, 1);
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
