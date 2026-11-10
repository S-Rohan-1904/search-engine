#include "prior.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <unordered_map>

PageRankPrior::PageRankPrior(const std::vector<RankedPage>& ranking, const InvertedIndex& index)
    : relative_(index.document_count(), 0.0) {
    std::unordered_map<std::string, std::size_t> doc_ids;
    const std::vector<std::string>& ids = index.document_ids();
    doc_ids.reserve(ids.size());
    for (std::size_t i = 0; i < ids.size(); i++) {
        doc_ids.emplace(ids[i], i);
    }

    const double pages = static_cast<double>(ranking.size());
    for (const RankedPage& page : ranking) {
        const auto found = doc_ids.find(page.page);
        if (found == doc_ids.end()) {
            continue;
        }
        relative_[found->second] = pages * page.score;
        covered_++;
    }
}

std::optional<PageRankPrior> PageRankPrior::load(const std::filesystem::path& path,
                                                 const InvertedIndex& index, std::string& error) {
    const std::optional<std::vector<RankedPage>> ranking = read_pagerank_file(path, error);
    if (!ranking.has_value()) {
        return std::nullopt;
    }
    return PageRankPrior(*ranking, index);
}

double PageRankPrior::boost(std::size_t doc_id, double weight) const {
    if (doc_id >= relative_.size()) {
        return 1.0;
    }
    return 1.0 + weight * std::log1p(relative_[doc_id]);
}

std::size_t PageRankPrior::covered() const {
    return covered_;
}

bool read_links(const std::filesystem::path& path,
                std::vector<std::pair<std::string, std::string>>& links, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "cannot read " + path.string();
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const std::size_t tab = line.find('\t');
        if (tab != std::string::npos) {
            links.emplace_back(line.substr(0, tab), line.substr(tab + 1));
        }
    }
    return true;
}

bool write_pagerank_file(const std::filesystem::path& path,
                         const std::vector<RankedPage>& ranking, std::string& error) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
        error = "cannot write " + path.string();
        return false;
    }

    out << std::setprecision(std::numeric_limits<double>::max_digits10);
    for (const RankedPage& page : ranking) {
        out << page.page << '\t' << page.score << '\n';
    }

    out.flush();
    if (!out) {
        error = "cannot write " + path.string();
        return false;
    }
    return true;
}

std::optional<std::vector<RankedPage>> read_pagerank_file(const std::filesystem::path& path,
                                                          std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "cannot read " + path.string();
        return std::nullopt;
    }

    std::vector<RankedPage> ranking;
    std::string line;
    std::size_t number = 0;
    while (std::getline(in, line)) {
        number++;
        if (line.empty()) {
            continue;
        }
        const std::size_t tab = line.rfind('\t');
        if (tab == std::string::npos) {
            error = path.string() + ":" + std::to_string(number) + ": expected page and score";
            return std::nullopt;
        }

        try {
            std::size_t consumed = 0;
            const std::string value = line.substr(tab + 1);
            const double score = std::stod(value, &consumed);
            if (consumed != value.size() || !std::isfinite(score) || score < 0.0) {
                throw std::invalid_argument("score");
            }
            ranking.push_back(RankedPage{line.substr(0, tab), score});
        } catch (const std::exception&) {
            error = path.string() + ":" + std::to_string(number) + ": bad score";
            return std::nullopt;
        }
    }
    return ranking;
}
