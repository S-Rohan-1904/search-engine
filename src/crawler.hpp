#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "fetcher.hpp"
#include "robots.hpp"
#include "url.hpp"

// What a crawl should do.
struct CrawlOptions {
    std::size_t max_pages = 50;
    std::size_t max_depth = 3;
    std::string user_agent = "searchbot";
    bool same_host_only = true;
    std::chrono::milliseconds min_delay{500};         // floor between requests to one host
    std::optional<std::filesystem::path> output_dir;  // write a corpus when set
};

// One page the crawl visited.
struct CrawledPage {
    std::string url;
    int status;
    std::string title;
    std::string text;
    std::size_t depth;
};

// How long to wait between two requests to one host.
//
// The site's Crawl-delay when it published one, but never less than `floor`.
// Taking the larger of the two means a site asking for a long delay gets it,
// while a site asking for a very short one does not talk this crawler into
// being ruder than it intends to be.
//
// Split out from the waiting itself because it is the part with a right answer:
// it can be tested without a clock, and sleeping cannot be tested without one.
std::chrono::milliseconds delay_for(const RobotsRules& rules, std::chrono::milliseconds floor);

// Crawls from `seed`, breadth first, and returns the pages it fetched.
//
// robots.txt is fetched once per host before anything else on that host, and a
// disallowed URL is never requested. A host whose robots.txt is missing is
// treated as permitting everything, which is what the standard says.
//
// Only HTML is parsed for links. Other content types are recorded and their
// text ignored, since a crawler that followed links out of a PDF or a
// stylesheet would wander in ways nobody expects.
//
// Requests to one host are spaced by delay_for(). Different hosts are not
// spaced against each other, since the politeness being bought is per-server.
//
// A page whose body is byte-identical to one already fetched is dropped rather
// than returned. Distinct URLs serving the same bytes are common (a directory
// and its index page, a tracking parameter, a printer-friendly variant), and
// indexing them twice inflates every one of their terms' document frequency and
// drags the average document length that BM25 divides by. Deduplication here is
// a ranking correctness measure, not a bandwidth one.
//
// When `output_dir` is set, each page is also written there in the corpus
// format, named doc_0001.txt and upward in visit order, so a crawl can be
// indexed directly.
std::vector<CrawledPage> crawl(const Url& seed, Fetcher& fetcher, const CrawlOptions& options);
