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

std::pair<std::uint64_t, std::size_t> body_fingerprint(std::string_view body) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const char c : body) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 1099511628211ULL;
    }
    return {hash, body.size()};
}

using Clock = std::chrono::steady_clock;

class HostRateLimiter {
public:
    Clock::time_point ready_at(const std::string& host) const {
        const auto entry = next_allowed_.find(host);
        return entry == next_allowed_.end() ? Clock::time_point{} : entry->second;
    }

    void record(const std::string& host, Clock::time_point request, std::chrono::milliseconds delay) {
        next_allowed_[host] = request + delay;
    }

private:
    std::unordered_map<std::string, Clock::time_point> next_allowed_;
};

struct FetchJob {
    FrontierEntry entry;
    std::optional<RobotsRules> rules;
    Clock::time_point not_before;
};

struct FetchOutcome {
    std::optional<RobotsRules> fetched_rules;
    bool allowed = true;
    Clock::time_point last_request;
    std::optional<FetchResult> result;
};

FetchOutcome run_job(const FetchJob& job, Fetcher& fetcher, const CrawlOptions& options) {
    FetchOutcome outcome;
    std::this_thread::sleep_until(job.not_before);

    RobotsRules rules;
    if (job.rules.has_value()) {
        rules = *job.rules;
    } else {
        Url robots_url = job.entry.url;
        robots_url.path = "/robots.txt";
        robots_url.query.clear();

        outcome.last_request = Clock::now();
        std::string text;
        const std::optional<FetchResult> robots = fetcher.get(robots_url);
        if (robots.has_value() && robots->status == 200) {
            text = robots->body;
        }
        rules = RobotsRules::parse(text, options.user_agent);
        outcome.fetched_rules = rules;

        if (!rules.allows(job.entry.url.path)) {
            outcome.allowed = false;
            return outcome;
        }
        if (options.min_delay.count() > 0) {
            std::this_thread::sleep_until(outcome.last_request + options.min_delay);
        }
    }

    outcome.last_request = Clock::now();
    outcome.result = fetcher.get(job.entry.url);
    return outcome;
}

std::string document_name(std::size_t ordinal) {
    std::ostringstream name;
    name << "doc_" << std::setw(4) << std::setfill('0') << ordinal;
    return name.str();
}

class LinkGraphWriter {
public:
    explicit LinkGraphWriter(const std::filesystem::path& directory)
        : links_(directory / "links.tsv", std::ios::trunc),
          urls_(directory / "urls.tsv", std::ios::trunc) {}

    void add(std::size_t ordinal, const std::string& url, const std::vector<std::string>& targets) {
        document_of_.emplace(url, ordinal);
        urls_ << document_name(ordinal) << "\t" << url << "\n";

        for (const std::string& target : targets) {
            const auto known = document_of_.find(target);
            if (known != document_of_.end()) {
                edge(ordinal, known->second);
            } else {
                waiting_[target].push_back(ordinal);
            }
        }

        const auto waiting = waiting_.find(url);
        if (waiting != waiting_.end()) {
            for (const std::size_t from : waiting->second) {
                edge(from, ordinal);
            }
            waiting_.erase(waiting);
        }

        links_.flush();
        urls_.flush();
    }

private:
    void edge(std::size_t from, std::size_t to) {
        links_ << document_name(from) << "\t" << document_name(to) << "\n";
    }

    std::ofstream links_;
    std::ofstream urls_;
    std::unordered_map<std::string, std::size_t> document_of_;
    std::unordered_map<std::string, std::vector<std::size_t>> waiting_;
};

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
    HostRateLimiter limiter;
    std::vector<CrawledPage> pages;

    std::optional<LinkGraphWriter> graph;
    if (options.output_dir.has_value()) {
        std::error_code ec;
        std::filesystem::create_directories(*options.output_dir, ec);
        graph.emplace(*options.output_dir);
    }

    const std::size_t workers = std::max<std::size_t>(1, options.concurrency);
    std::vector<std::unique_ptr<Fetcher>> clones;
    std::vector<Fetcher*> fetchers{&fetcher};
    for (std::size_t i = 1; i < workers; i++) {
        clones.push_back(fetcher.clone());
        fetchers.push_back(clones.back().get());
    }

    std::unordered_map<std::string, std::size_t> queued_per_host;
    frontier.push(seed, 0);
    queued_per_host[seed.host] = 1;

    const auto job_for = [&](FrontierEntry entry) {
        const auto known = robots.find(entry.url.host);
        const Clock::time_point ready = limiter.ready_at(entry.url.host);
        FetchJob job{std::move(entry), std::nullopt, ready};
        if (known != robots.end()) {
            job.rules = known->second;
        }
        return job;
    };

    while (!frontier.empty()) {
        std::vector<FetchJob> jobs;
        if (workers == 1) {
            if (std::optional<FrontierEntry> entry = frontier.pop()) {
                jobs.push_back(job_for(std::move(*entry)));
            }
        } else {
            const Clock::time_point now = Clock::now();
            std::set<std::string> hosts;
            while (jobs.size() < workers) {
                std::optional<FrontierEntry> entry =
                    frontier.pop_first([&](const FrontierEntry& candidate) {
                        const std::string& host = candidate.url.host;
                        return hosts.count(host) == 0 && limiter.ready_at(host) <= now;
                    });
                if (!entry.has_value()) {
                    break;
                }
                hosts.insert(entry->url.host);
                jobs.push_back(job_for(std::move(*entry)));
            }
            if (jobs.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                continue;
            }
        }

        std::vector<FetchOutcome> outcomes(jobs.size());
        for (FetchJob& job : jobs) {
            if (job.rules.has_value() && !job.rules->allows(job.entry.url.path)) {
                job.not_before = Clock::time_point::max();
            }
        }
        if (jobs.size() == 1) {
            if (jobs.front().not_before == Clock::time_point::max()) {
                outcomes.front().allowed = false;
            } else {
                outcomes.front() = run_job(jobs.front(), *fetchers.front(), options);
            }
        } else {
            std::vector<std::thread> threads;
            for (std::size_t i = 0; i < jobs.size(); i++) {
                if (jobs[i].not_before == Clock::time_point::max()) {
                    outcomes[i].allowed = false;
                    continue;
                }
                threads.emplace_back([&, i] { outcomes[i] = run_job(jobs[i], *fetchers[i], options); });
            }
            for (std::thread& thread : threads) {
                thread.join();
            }
        }

        for (std::size_t i = 0; i < jobs.size(); i++) {
            const FrontierEntry* entry = &jobs[i].entry;
            const Url& url = entry->url;
            FetchOutcome& outcome = outcomes[i];

            if (outcome.fetched_rules.has_value()) {
                robots.emplace(url.host, *outcome.fetched_rules);
            }
            if (!outcome.allowed) {
                if (outcome.fetched_rules.has_value()) {
                    limiter.record(url.host, outcome.last_request, options.min_delay);
                }
                continue;
            }
            limiter.record(url.host, outcome.last_request,
                           delay_for(robots.at(url.host), options.min_delay));

            const std::optional<FetchResult>& result = outcome.result;
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
                    if (!target->path.starts_with(options.path_prefix)) {
                        continue;
                    }
                    outgoing.push_back(url_to_string(*target));
                    std::size_t& queued = queued_per_host[target->host];
                    if (options.max_per_host != 0 && queued >= options.max_per_host) {
                        continue;
                    }
                    if (frontier.push(*target, entry->depth + 1)) {
                        queued++;
                    }
                }
            }

            pages.push_back(std::move(page));

            if (options.output_dir.has_value() && result->status == 200) {
                const std::size_t ordinal = pages.size();
                write_page(*options.output_dir, ordinal, pages.back());
                graph->add(ordinal, pages.back().url, outgoing);
            }
        }
    }

    return pages;
}
