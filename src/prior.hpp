#pragma once

#include <cstddef>
#include <filesystem>
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "index.hpp"
#include "pagerank.hpp"

// Which link graph the PageRank prior comes from. `Page` ranks every crawled
// page by the links between pages. `Site` ranks hosts by the links between
// different hosts and credits each host's score to its home page; see
// siterank.hpp. `Home` reads no links: every home page gets the same score,
// the control that showed most of the site-level gain is preferring home
// pages at all.
enum class LinkGraph { Page, Site, Home };

// "page", "site" or "home", as the flags, REPL and URLs spell them.
std::optional<LinkGraph> parse_link_graph(std::string_view text);
const char* link_graph_name(LinkGraph graph);

class PageRankPrior {
public:
    PageRankPrior(const std::vector<RankedPage>& ranking, const InvertedIndex& index);

    static std::optional<PageRankPrior> load(const std::filesystem::path& path,
                                             const InvertedIndex& index, std::string& error);

    double boost(std::size_t doc_id, double weight) const;

    std::size_t covered() const;

private:
    std::vector<double> relative_;
    std::size_t covered_ = 0;
};

// The priors a source has, at most one per graph. A REPL session or a server
// switches graph per query, so every prior the source can supply is loaded
// once up front and looked up here; a graph the source has nothing for is
// simply absent.
class PriorSet {
public:
    void add(LinkGraph graph, PageRankPrior prior);

    // Null when the source has no prior for this graph.
    const PageRankPrior* find(LinkGraph graph) const;

private:
    std::array<std::optional<PageRankPrior>, 3> priors_;
};

bool read_links(const std::filesystem::path& path,
                std::vector<std::pair<std::string, std::string>>& links, std::string& error);

bool write_pagerank_file(const std::filesystem::path& path,
                         const std::vector<RankedPage>& ranking, std::string& error);

std::optional<std::vector<RankedPage>> read_pagerank_file(const std::filesystem::path& path,
                                                          std::string& error);
