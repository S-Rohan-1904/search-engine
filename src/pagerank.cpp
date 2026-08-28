#include "pagerank.hpp"

#include <algorithm>
#include <set>
#include <unordered_map>

std::vector<RankedPage> pagerank(const std::vector<std::pair<std::string, std::string>>& links,
                                 std::size_t iterations, double damping) {
    std::vector<std::string> pages;
    std::unordered_map<std::string, std::size_t> ordinal;

    const auto intern = [&](const std::string& name) {
        const auto existing = ordinal.find(name);
        if (existing != ordinal.end()) {
            return existing->second;
        }
        ordinal.emplace(name, pages.size());
        pages.push_back(name);
        return pages.size() - 1;
    };

    std::set<std::pair<std::size_t, std::size_t>> unique_edges;
    for (const auto& [from, to] : links) {
        unique_edges.emplace(intern(from), intern(to));
    }
    const std::vector<std::pair<std::size_t, std::size_t>> edges(unique_edges.begin(),
                                                                 unique_edges.end());

    const std::size_t count = pages.size();
    if (count == 0) {
        return {};
    }

    std::vector<std::size_t> outgoing(count, 0);
    for (const auto& [from, to] : edges) {
        (void)to;
        outgoing[from]++;
    }

    const double total = static_cast<double>(count);
    std::vector<double> score(count, 1.0 / total);
    std::vector<double> next(count, 0.0);

    for (std::size_t round = 0; round < iterations; round++) {
        double dangling = 0.0;
        for (std::size_t i = 0; i < count; i++) {
            if (outgoing[i] == 0) {
                dangling += score[i];
            }
        }

        const double base = (1.0 - damping) / total + damping * dangling / total;
        std::fill(next.begin(), next.end(), base);

        for (const auto& [from, to] : edges) {
            next[to] += damping * score[from] / static_cast<double>(outgoing[from]);
        }

        score.swap(next);
    }

    std::vector<RankedPage> out;
    out.reserve(count);
    for (std::size_t i = 0; i < count; i++) {
        out.push_back(RankedPage{pages[i], score[i]});
    }

    std::sort(out.begin(), out.end(), [](const RankedPage& a, const RankedPage& b) {
        if (a.score != b.score) {
            return a.score > b.score;
        }
        return a.page < b.page;
    });
    return out;
}
