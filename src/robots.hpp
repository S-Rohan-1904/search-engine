#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// One rule from a robots.txt group.
struct RobotsRule {
    std::string pattern;
    bool allow;
};

// The rules that apply to one crawler.
//
// robots.txt is a list of groups, each naming the user agents it covers and
// then the paths they may and may not fetch. A crawler reads the file once,
// keeps the group that matches its own name, and consults it before every
// request.
class RobotsRules {
public:
    // Parses robots.txt and keeps the group matching `user_agent`.
    //
    // Matching is case-insensitive and by prefix, as the standard requires, so
    // a group for "searchbot" applies to "SearchBot/1.0". The "*" group is used
    // only when no group names this crawler specifically, since a named group
    // overrides the wildcard entirely rather than adding to it.
    //
    // A file that cannot be parsed, or names no matching group, permits
    // everything. That is the standard's default: robots.txt restricts a
    // crawler, and its absence restricts nothing.
    static RobotsRules parse(std::string_view text, std::string_view user_agent);

    // Whether `path` may be fetched.
    //
    // The longest matching pattern wins, and Allow wins a tie. That ordering is
    // what lets a site disallow a directory and then permit one page inside it.
    //
    // Patterns match by prefix, with two wildcards: '*' matches any run of
    // characters, and a trailing '$' anchors the match to the end of the path.
    bool allows(std::string_view path) const;

    // The site's requested delay between requests, when it asked for one.
    //
    // Crawl-delay is not part of the original robots.txt standard and not every
    // crawler honours it, but a site that publishes a number has stated what it
    // can tolerate, and there is no reason to argue. Fractional seconds are
    // allowed, so the value is kept in milliseconds.
    std::optional<std::chrono::milliseconds> crawl_delay() const { return crawl_delay_; }

    const std::vector<RobotsRule>& rules() const { return rules_; }

private:
    std::vector<RobotsRule> rules_;
    std::optional<std::chrono::milliseconds> crawl_delay_;
};
