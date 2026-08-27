#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "index.hpp"

// A document and the score a ranking function gave it.
struct ScoredDocument {
    std::size_t doc_id;
    double score;
};

// Analyzes the words of a query into the terms a scorer works on.
//
// Words that analyze away contribute nothing. A repeated word is kept, and so
// counted twice, which is how a searcher emphasises it.
std::vector<std::string> query_terms(const std::vector<std::string>& words);

// Scores every document containing at least one query term, highest first,
// breaking ties by ascending document id.
//
// TF-IDF, with a logarithmic term frequency and an inverse document frequency
// of ln(N / df):
//
//     score(d) = sum over terms of (1 + ln tf) * ln(N / df)
//
// The tf part saturates because relevance does: a document naming a term
// twenty times is not twenty times more about it than one naming it once. The
// idf part is what makes a rare term worth more than a common one, and it
// reaches zero for a term present in every document, which then cannot
// discriminate between any of them.
//
// Nothing here accounts for document length, so a long document outscores a
// short one that is more focused. BM25 fixes that.
std::vector<ScoredDocument> score_tfidf(const std::vector<std::string>& terms,
                                        const InvertedIndex& index);

// Scores every document containing at least one query term, ordered as above.
//
// BM25, with the usual k1 = 1.2 and b = 0.75:
//
//     score(d) = sum over terms of idf * (tf * (k1 + 1))
//                                / (tf + k1 * (1 - b + b * len / avglen))
//
// Two things differ from TF-IDF. The term frequency approaches a ceiling of
// k1 + 1 rather than growing without bound, so no single term can dominate.
// And the denominator carries the document's length relative to the corpus
// average, so a term in a short document counts for more than the same term
// buried in a long one.
//
// The idf is ln(1 + (N - df + 0.5) / (df + 0.5)), which stays positive even
// for a term that occurs in every document.
std::vector<ScoredDocument> score_bm25(const std::vector<std::string>& terms,
                                       const InvertedIndex& index);

// Which scoring function to use.
enum class Scorer {
    TfIdf,
    Bm25,
};

// Scores on several threads, with a result identical to the single-threaded
// scorers for any thread count.
//
// The document id space is split into contiguous shards, one per thread, and
// each shard is scored independently. Sharding by document rather than by term
// is what keeps the result identical: a document's score is summed by exactly
// one thread, in the same term order every time, so no floating-point addition
// ever changes order. Splitting the terms across threads instead would require
// adding partial sums together, and floating-point addition is not associative,
// so the last bits would depend on the thread count.
std::vector<ScoredDocument> score(const std::vector<std::string>& terms,
                                  const InvertedIndex& index, Scorer scorer,
                                  std::size_t threads);

// The k highest-scoring entries, in the same order the scorers produce.
//
// Keeps a min-heap of the k best seen so far, so the cost is O(n log k) rather
// than the O(n log n) of sorting everything. For k = 10 over a million scored
// documents that is the difference between ten comparisons per document and
// twenty, and between holding ten results in memory and a million.
//
// k of 0 returns nothing; a k at or above the input size returns everything,
// fully ordered.
std::vector<ScoredDocument> top_k(const std::vector<ScoredDocument>& scored, std::size_t k);
