#include "siterank.hpp"

#include <algorithm>
#include <optional>
#include <unordered_map>

#include "prior.hpp"
#include "url.hpp"

namespace {

struct Sites {
    std::unordered_map<std::string, std::string> host_of;
    std::unordered_map<std::string, std::string> home_of;
};

Sites map_sites(const std::vector<std::pair<std::string, std::string>>& urls) {
    Sites sites;
    sites.host_of.reserve(urls.size());
    for (const auto& [page, text] : urls) {
        const std::optional<Url> url = parse_url(text);
        if (!url.has_value()) {
            continue;
        }
        sites.host_of.emplace(page, url->host);
        if (url->path == "/" && url->query.empty()) {
            sites.home_of.emplace(url->host, page);
        }
    }
    return sites;
}

}

std::vector<RankedPage> site_pagerank(const std::vector<std::pair<std::string, std::string>>& links,
                                      const std::vector<std::pair<std::string, std::string>>& urls,
                                      std::size_t iterations, double damping) {
    const Sites sites = map_sites(urls);
    const auto& host_of = sites.host_of;
    const auto& home_of = sites.home_of;

    std::vector<std::pair<std::string, std::string>> host_links;
    for (const auto& [from, to] : links) {
        const auto source = host_of.find(from);
        const auto target = host_of.find(to);
        if (source == host_of.end() || target == host_of.end() ||
            source->second == target->second) {
            continue;
        }
        host_links.emplace_back(source->second, target->second);
    }

    std::vector<RankedPage> credited;
    for (const RankedPage& host : pagerank(host_links, iterations, damping)) {
        const auto home = home_of.find(host.page);
        if (home != home_of.end()) {
            credited.push_back(RankedPage{home->second, host.score});
        }
    }
    return credited;
}

std::vector<RankedPage> home_page_ranking(
    const std::vector<std::pair<std::string, std::string>>& urls) {
    std::vector<RankedPage> homes;
    for (const auto& [host, page] : map_sites(urls).home_of) {
        homes.push_back(RankedPage{page, 0.0});
    }
    std::sort(homes.begin(), homes.end(),
              [](const RankedPage& a, const RankedPage& b) { return a.page < b.page; });
    for (RankedPage& home : homes) {
        home.score = 1.0 / static_cast<double>(homes.size());
    }
    return homes;
}

bool read_page_urls(const std::filesystem::path& path,
                    std::vector<std::pair<std::string, std::string>>& urls, std::string& error) {
    return read_links(path, urls, error);
}
