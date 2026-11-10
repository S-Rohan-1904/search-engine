#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "index.hpp"
#include "pagerank.hpp"

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

bool read_links(const std::filesystem::path& path,
                std::vector<std::pair<std::string, std::string>>& links, std::string& error);

bool write_pagerank_file(const std::filesystem::path& path,
                         const std::vector<RankedPage>& ranking, std::string& error);

std::optional<std::vector<RankedPage>> read_pagerank_file(const std::filesystem::path& path,
                                                          std::string& error);
