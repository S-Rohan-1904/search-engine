#pragma once

#include <optional>
#include <string>
#include <unordered_map>

#include "complete.hpp"
#include "corpus.hpp"
#include "http.hpp"
#include "index.hpp"
#include "prior.hpp"
#include "results.hpp"
#include "suggest.hpp"

class SearchService {
public:
    SearchService(const InvertedIndex& index, const CorpusReader* corpus,
                  const PageRankPrior* prior,
                  std::unordered_map<std::string, std::string> urls, QueryOptions defaults);

    HttpResponse handle(const HttpRequest& request);

private:
    struct Answer {
        std::string query;
        std::string error;
        std::optional<std::string> suggestion;
        std::vector<QueryResult> results;
        QueryOptions options;
    };

    Answer answer(const HttpRequest& request);
    std::string title_of(const std::string& document_id) const;
    std::string url_of(const std::string& document_id) const;
    HttpResponse html(const Answer& answer) const;
    HttpResponse json(const Answer& answer) const;
    const BkTree& tree();
    const Trie& trie();
    HttpResponse completions(const HttpRequest& request);

    const InvertedIndex& index_;
    const CorpusReader* corpus_;
    const PageRankPrior* prior_;
    std::unordered_map<std::string, std::string> urls_;
    QueryOptions defaults_;
    std::optional<BkTree> tree_;
    std::optional<Trie> trie_;
};

std::unordered_map<std::string, std::string> read_urls(const std::string& path);

bool serve(SearchService& service, int port, std::string& error);
