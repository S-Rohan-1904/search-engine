#include "siterank.hpp"

#include <optional>
#include <unordered_map>

#include "prior.hpp"
#include "url.hpp"

std::vector<RankedPage> site_pagerank(const std::vector<std::pair<std::string, std::string>>& links,
                                      const std::vector<std::pair<std::string, std::string>>& urls,
                                      std::size_t iterations, double damping) {
    std::unordered_map<std::string, std::string> host_of;
    std::unordered_map<std::string, std::string> home_of;
    host_of.reserve(urls.size());
    for (const auto& [page, text] : urls) {
        const std::optional<Url> url = parse_url(text);
        if (!url.has_value()) {
            continue;
        }
        host_of.emplace(page, url->host);
        if (url->path == "/" && url->query.empty()) {
            home_of.emplace(url->host, page);
        }
    }

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

bool read_page_urls(const std::filesystem::path& path,
                    std::vector<std::pair<std::string, std::string>>& urls, std::string& error) {
    return read_links(path, urls, error);
}
