#pragma once

#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "index.hpp"
#include "ranking.hpp"

// How a search should be run and shown.
struct QueryOptions {
    std::size_t limit = 10;
    Scorer scorer = Scorer::Bm25;
    bool snippets = false;
    std::size_t snippet_chars = 160;
    bool tsv = false;
};

// One result, ready to print.
struct QueryResult {
    std::string document_id;
    double score;
    std::string snippet;  // empty unless snippets were asked for and available
};

// Runs a query and returns the results it should show.
//
// The boolean expression decides *which* documents match and the scorer decides
// what order they come in, which is how a real engine separates the two. A
// document matching through NOT alone contains none of the query terms and
// therefore scores zero; those sort last rather than being dropped, since they
// are legitimate matches.
//
// `corpus_dir` is needed only for snippets, which are cut from the document
// text the index does not store. Without it the results still come back, with
// their snippets empty.
//
// Returns nullopt with `error` set when the query does not parse.
std::optional<std::vector<QueryResult>> run_query(
    const InvertedIndex& index, const std::optional<std::filesystem::path>& corpus_dir,
    std::string_view query, const QueryOptions& options, std::string& error);

// Writes results in the chosen format.
//
// The default is meant to be read: a rank, the document id, its score, and the
// snippet indented beneath. `tsv` is meant to be piped: id and score separated
// by a tab, one line each, no ranks and no blank lines.
void print_results(std::ostream& out, const std::vector<QueryResult>& results,
                   const QueryOptions& options);
