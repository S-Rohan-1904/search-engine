#include "crawler.hpp"

#include <fstream>
#include <cstdint>
#include <set>
#include <thread>
#include <utility>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <unordered_map>

#include "frontier.hpp"
#include "html.hpp"
#include "robots.hpp"

namespace {

class HostRateLimiter;

const RobotsRules& rules_for_host(std::unordered_map<std::string, RobotsRules>& cache,
                                  const Url& url, Fetcher& fetcher,
                                  const std::string& user_agent,
                                  HostRateLimiter& limiter, std::chrono::milliseconds floor);

std::pair<std::uint64_t, std::size_t> body_fingerprint(std::string_view body) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const char c : body) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 1099511628211ULL;
    }
    return {hash, body.size()};
}

class HostRateLimiter {
public:
    void wait_for(const std::string& host, std::chrono::milliseconds delay) {
        if (delay.count() <= 0) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto entry = next_allowed_.find(host);
        if (entry != next_allowed_.end() && entry->second > now) {
            std::this_thread::sleep_for(entry->second - now);
        }

        next_allowed_[host] = std::chrono::steady_clock::now() + delay;
    }

private:
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> next_allowed_;
};

const RobotsRules& rules_for_host(std::unordered_map<std::string, RobotsRules>& cache,
                                  const Url& url, Fetcher& fetcher,
                                  const std::string& user_agent,
                                  HostRateLimiter& limiter, std::chrono::milliseconds floor) {
    const auto existing = cache.find(url.host);
    if (existing != cache.end()) {
        return existing->second;
    }

    limiter.wait_for(url.host, floor);

    Url robots_url = url;
    robots_url.path = "/robots.txt";
    robots_url.query.clear();

    std::string text;
    const std::optional<FetchResult> result = fetcher.get(robots_url);
    if (result.has_value() && result->status == 200) {
        text = result->body;
    }

    return cache.emplace(url.host, RobotsRules::parse(text, user_agent)).first->second;
}

void write_page(const std::filesystem::path& directory, std::size_t ordinal,
                const CrawledPage& page) {
    std::ostringstream name;
    name << "doc_" << std::setw(4) << std::setfill('0') << ordinal << ".txt";

    std::ofstream out(directory / name.str());
    if (!out) {
        return;
    }

    out << "title: " << (page.title.empty() ? page.url : page.title) << "\n\n" << page.text << "\n";
}

}

std::chrono::milliseconds delay_for(const RobotsRules& rules, std::chrono::milliseconds floor) {
    const std::optional<std::chrono::milliseconds> requested = rules.crawl_delay();
    if (!requested.has_value()) {
        return floor;
    }
    return std::max(*requested, floor);
}

std::vector<CrawledPage> crawl(const Url& seed, Fetcher& fetcher, const CrawlOptions& options) {
    Frontier frontier(options.max_depth, options.max_pages);
    std::unordered_map<std::string, RobotsRules> robots;
    std::set<std::pair<std::uint64_t, std::size_t>> fingerprints;
    HostRateLimiter limiter;
    std::vector<CrawledPage> pages;

    if (options.output_dir.has_value()) {
        std::error_code ec;
        std::filesystem::create_directories(*options.output_dir, ec);
    }

    frontier.push(seed, 0);

    while (const std::optional<FrontierEntry> entry = frontier.pop()) {
        const Url& url = entry->url;

        const RobotsRules& rules =
            rules_for_host(robots, url, fetcher, options.user_agent, limiter, options.min_delay);
        if (!rules.allows(url.path)) {
            continue;
        }

        limiter.wait_for(url.host, delay_for(rules, options.min_delay));

        const std::optional<FetchResult> result = fetcher.get(url);
        if (!result.has_value()) {
            continue;
        }

        if (result->status == 200 &&
            !fingerprints.insert(body_fingerprint(result->body)).second) {
            continue;
        }

        CrawledPage page;
        page.url = url_to_string(url);
        page.status = result->status;
        page.depth = entry->depth;

        if (result->status == 200 && result->content_type.starts_with("text/html")) {
            const HtmlPage parsed = parse_html(result->body);
            page.title = parsed.title;
            page.text = parsed.text;

            for (const std::string& href : parsed.links) {
                const std::optional<Url> target = resolve_url(url, href);
                if (!target.has_value()) {
                    continue;
                }
                if (options.same_host_only && target->host != seed.host) {
                    continue;
                }
                frontier.push(*target, entry->depth + 1);
            }
        }

        pages.push_back(std::move(page));

        if (options.output_dir.has_value() && result->status == 200) {
            write_page(*options.output_dir, pages.size(), pages.back());
        }
    }

    return pages;
}
