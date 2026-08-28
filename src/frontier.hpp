#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <string>
#include <unordered_set>

#include "url.hpp"

// A URL waiting to be fetched, and how many links away from a seed it is.
struct FrontierEntry {
    Url url;
    std::size_t depth;
};

// The crawler's queue of work.
//
// A FIFO, so the crawl proceeds breadth first: every page one link from a seed
// is fetched before any page two links away. That ordering matters because the
// pages nearest a seed are usually the ones a site considers important, and a
// crawl that stops early should stop having collected those rather than having
// followed one chain to its end.
//
// The frontier also owns the three things that keep a crawl finite: it refuses
// a URL it has already seen, one beyond the depth limit, and any URL at all
// once the page limit is reached.
class Frontier {
public:
    Frontier(std::size_t max_depth, std::size_t max_pages)
        : max_depth_(max_depth), max_pages_(max_pages) {}

    // Offers a URL to the queue. Returns whether it was accepted.
    //
    // Deduplication is on the canonical form, so "HTTP://Example.com" and
    // "http://example.com/" are the same page and the second is refused. The
    // seen set holds every URL ever offered, not just those fetched, so a link
    // appearing on fifty pages is queued once.
    bool push(const Url& url, std::size_t depth);

    std::optional<FrontierEntry> pop();

    // How many URLs have been handed out by pop().
    std::size_t issued() const { return issued_; }

    std::size_t seen() const { return seen_.size(); }

private:
    std::deque<FrontierEntry> queue_;
    std::unordered_set<std::string> seen_;
    std::size_t max_depth_;
    std::size_t max_pages_;
    std::size_t issued_ = 0;
};
