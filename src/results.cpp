#include "results.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <unordered_map>
#include <unordered_set>

#include "document.hpp"
#include "evaluator.hpp"
#include "query.hpp"
#include "snippet.hpp"

std::optional<std::vector<QueryResult>> run_query(
    const InvertedIndex& index, const CorpusReader* corpus, std::string_view query,
    const QueryOptions& options, std::string& error) {
    error.clear();

    const ParseResult parsed = parse_query(query);
    if (!parsed.root) {
        error = parsed.error;
        return std::nullopt;
    }

    const std::vector<std::size_t> matches = evaluate(*parsed.root, index);
    const std::unordered_set<std::size_t> matched(matches.begin(), matches.end());

    std::vector<std::string> terms;
    for (const QueryToken& token : [&] {
             std::vector<QueryToken> tokens;
             std::string ignored;
             lex_query(query, tokens, ignored);
             return tokens;
         }()) {
        if (token.kind == QueryTokenKind::Term || token.kind == QueryTokenKind::Phrase) {
            for (const std::string& term : query_terms({token.text})) {
                terms.push_back(term);
            }
        }
    }

    std::unordered_map<std::size_t, double> scored;
    for (const ScoredDocument& document : score(terms, index, options.scorer, 1)) {
        if (matched.count(document.doc_id) != 0) {
            scored.emplace(document.doc_id, document.score);
        }
    }

    std::vector<ScoredDocument> ranking;
    ranking.reserve(matches.size());
    for (const std::size_t doc_id : matches) {
        const auto found = scored.find(doc_id);
        ranking.push_back(ScoredDocument{doc_id, found == scored.end() ? 0.0 : found->second});
    }

    const std::vector<ScoredDocument> best = top_k(ranking, options.limit);
    const std::vector<std::string>& ids = index.document_ids();

    std::vector<QueryResult> results;
    results.reserve(best.size());
    for (const ScoredDocument& document : best) {
        QueryResult result{ids[document.doc_id], document.score, ""};

        if (options.snippets && corpus != nullptr) {
            const std::optional<Document> doc = corpus->read(result.document_id);
            if (doc.has_value()) {
                SnippetOptions snippet_options;
                snippet_options.max_chars = options.snippet_chars;
                result.snippet = make_snippet(doc->title + "\n\n" + doc->body, terms,
                                              snippet_options);
            }
        }

        results.push_back(std::move(result));
    }

    return results;
}

void print_results(std::ostream& out, const std::vector<QueryResult>& results,
                   const QueryOptions& options) {
    if (options.tsv) {
        out << std::fixed << std::setprecision(4);
        for (const QueryResult& result : results) {
            out << result.document_id << "\t" << result.score << "\n";
        }
        return;
    }

    out << std::fixed << std::setprecision(4);
    for (std::size_t i = 0; i < results.size(); i++) {
        out << (i + 1) << ". " << results[i].document_id << "  " << results[i].score << "\n";
        if (!results[i].snippet.empty()) {
            std::string flattened;
            bool space = false;
            for (const char c : results[i].snippet) {
                if (c == '\n' || c == '\r' || c == '\t') {
                    space = true;
                    continue;
                }
                if (space && !flattened.empty()) {
                    flattened.push_back(' ');
                }
                space = false;
                flattened.push_back(c);
            }
            out << "   " << flattened << "\n";
        }
    }
}
