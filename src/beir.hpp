#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

#include "index.hpp"
#include "results.hpp"

// Evaluating ranking against a collection someone else judged.
//
// Precision measured against relevance judgements written for the engine being
// measured is not evidence of anything. BEIR is a public benchmark: a corpus, a
// query set, and qrels produced independently, with BM25 baselines published
// per dataset. Scoring against those makes the number checkable by someone who
// does not trust this code, and comparable to a reference implementation.
//
// The layout on disk is BEIR's own:
//
//     corpus.jsonl     {"_id": ..., "title": ..., "text": ...}
//     queries.jsonl    {"_id": ..., "text": ...}
//     qrels/test.tsv   query-id <tab> corpus-id <tab> score, with a header row

struct BeirImportReport {
    std::size_t lines_seen = 0;
    std::size_t documents_written = 0;
    std::size_t skipped_empty = 0;
};

// Converts a BEIR corpus.jsonl into a .corpus file.
//
// Document ids are BEIR's own, not ordinals, because the qrels name them. The
// corpus format requires ids in sorted order, so the ids are read, sorted, and
// the documents written in that order; a BEIR corpus is small enough for that
// to be reasonable.
bool import_beir(const std::filesystem::path& corpus_jsonl, const std::filesystem::path& output,
                 BeirImportReport& report, std::string& error);

// One query and the documents judged relevant to it.
struct JudgedQuery {
    std::string id;
    std::string text;
};

struct RelevanceReport {
    std::size_t queries = 0;      // queries with at least one judged document
    std::size_t unjudged = 0;     // queries in the file with no qrels
    double precision_at_10 = 0.0;
    double recall_at_100 = 0.0;
    double ndcg_at_10 = 0.0;
};

// Runs every judged query and scores the ranking against the qrels.
//
// nDCG@10 with graded gains, which is what BEIR reports, so the number can be
// put next to the published baseline. Precision@10 alongside it, because it is
// the one an interviewer will ask about and it does not need explaining.
bool evaluate_relevance(const InvertedIndex& index, const std::filesystem::path& queries_jsonl,
                        const std::filesystem::path& qrels_tsv, const QueryOptions& options,
                        RelevanceReport& report, std::string& error);
