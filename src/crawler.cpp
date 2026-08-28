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

std::string document_name(std::size_t ordinal) {
    std::ostringstream name;
    name << "doc_" << std::setw(4) << std::setfill('0') << ordinal;
    return name.str();
}

void write_links(const std::filesystem::path& directory,
                 const std::unordered_map<std::string, std::size_t>& document_of,
                 const std::vector<std::pair<std::size_t, std::string>>& links) {
    std::ofstream out(directory / "links.tsv");
    if (!out) {
        return;
    }

    for (const auto& [from, target] : links) {
        const auto to = document_of.find(target);
        if (to != document_of.end()) {
            out << document_name(from) << "\t" << document_name(to->second) << "\n";
        }
    }
}

void write_page(const std::filesystem::path& directory, std::size_t ordinal,
                const CrawledPage& page) {
    std::ofstream out(directory / (document_name(ordinal) + ".txt"));
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
    std::unordered_map<std::string, std::size_t> document_of;
    std::vector<std::pair<std::size_t, std::string>> link_pairs;
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

        std::vector<std::string> outgoing;
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
                outgoing.push_back(url_to_string(*target));
                frontier.push(*target, entry->depth + 1);
            }
        }

        pages.push_back(std::move(page));

        if (options.output_dir.has_value() && result->status == 200) {
            const std::size_t ordinal = pages.size();
            write_page(*options.output_dir, ordinal, pages.back());
            document_of[pages.back().url] = ordinal;
            for (std::string& target : outgoing) {
                link_pairs.emplace_back(ordinal, std::move(target));
            }
        }
    }

    if (options.output_dir.has_value()) {
        write_links(*options.output_dir, document_of, link_pairs);
    }

    return pages;
}
