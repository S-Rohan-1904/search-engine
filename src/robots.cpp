#include "robots.hpp"

#include <cctype>
#include <cstdlib>

namespace {

std::string lowercase(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

std::string_view trim(std::string_view text) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
        text.remove_prefix(1);
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) {
        text.remove_suffix(1);
    }
    return text;
}

bool matches_pattern(std::string_view pattern, std::string_view path) {
    bool anchored = false;
    if (!pattern.empty() && pattern.back() == '$') {
        anchored = true;
        pattern.remove_suffix(1);
    }

    std::vector<std::string_view> parts;
    std::size_t i = 0;
    while (true) {
        const std::size_t star = pattern.find('*', i);
        if (star == std::string_view::npos) {
            parts.push_back(pattern.substr(i));
            break;
        }
        parts.push_back(pattern.substr(i, star - i));
        i = star + 1;
    }

    std::size_t position = 0;
    for (std::size_t part = 0; part < parts.size(); part++) {
        const std::string_view piece = parts[part];

        if (part == 0) {
            if (path.compare(0, piece.size(), piece) != 0) {
                return false;
            }
            position = piece.size();
            continue;
        }

        if (part + 1 == parts.size() && anchored) {
            if (piece.size() > path.size() - position) {
                return false;
            }
            return path.compare(path.size() - piece.size(), piece.size(), piece) == 0;
        }

        if (piece.empty()) {
            continue;
        }

        const std::size_t found = path.find(piece, position);
        if (found == std::string_view::npos) {
            return false;
        }
        position = found + piece.size();
    }

    return !anchored || position == path.size();
}

std::size_t pattern_length(std::string_view pattern) {
    return pattern.size();
}

}

RobotsRules RobotsRules::parse(std::string_view text, std::string_view user_agent) {
    const std::string agent = lowercase(user_agent);

    std::vector<RobotsRule> wildcard;
    std::vector<RobotsRule> named;
    std::optional<std::chrono::milliseconds> wildcard_delay;
    std::optional<std::chrono::milliseconds> named_delay;
    std::optional<std::chrono::milliseconds> group_delay;
    std::vector<std::string> group_agents;
    bool in_rules = false;

    const auto flush = [&](std::vector<RobotsRule>& rules, const std::vector<std::string>& agents) {
        for (const std::string& candidate : agents) {
            if (candidate == "*") {
                wildcard.insert(wildcard.end(), rules.begin(), rules.end());
                if (group_delay.has_value()) {
                    wildcard_delay = group_delay;
                }
            } else if (agent.compare(0, candidate.size(), candidate) == 0) {
                named.insert(named.end(), rules.begin(), rules.end());
                if (group_delay.has_value()) {
                    named_delay = group_delay;
                }
            }
        }
        group_delay.reset();
    };

    std::vector<RobotsRule> group_rules;
    std::size_t line_start = 0;

    while (line_start <= text.size()) {
        const std::size_t line_end = text.find('\n', line_start);
        std::string_view line =
            text.substr(line_start, line_end == std::string_view::npos ? std::string_view::npos
                                                                      : line_end - line_start);

        const std::size_t comment = line.find('#');
        if (comment != std::string_view::npos) {
            line = line.substr(0, comment);
        }
        line = trim(line);

        const std::size_t colon = line.find(':');
        if (colon != std::string_view::npos) {
            const std::string field = lowercase(trim(line.substr(0, colon)));
            const std::string_view value = trim(line.substr(colon + 1));

            if (field == "user-agent") {
                if (in_rules) {
                    flush(group_rules, group_agents);
                    group_rules.clear();
                    group_agents.clear();
                    in_rules = false;
                }
                group_agents.push_back(lowercase(value));
            } else if (field == "crawl-delay") {
                in_rules = true;
                char* end = nullptr;
                const std::string number(value);
                const double seconds = std::strtod(number.c_str(), &end);
                if (end != number.c_str() && seconds >= 0.0) {
                    group_delay = std::chrono::milliseconds(static_cast<long long>(seconds * 1000));
                }
            } else if (field == "allow" || field == "disallow") {
                in_rules = true;
                if (!value.empty() || field == "allow") {
                    group_rules.push_back(RobotsRule{std::string(value), field == "allow"});
                }
            }
        }

        if (line_end == std::string_view::npos) {
            break;
        }
        line_start = line_end + 1;
    }

    flush(group_rules, group_agents);

    RobotsRules result;
    const bool use_named = !named.empty() || named_delay.has_value();
    result.rules_ = use_named ? named : wildcard;
    result.crawl_delay_ = use_named ? named_delay : wildcard_delay;
    return result;
}

bool RobotsRules::allows(std::string_view path) const {
    const RobotsRule* best = nullptr;

    for (const RobotsRule& rule : rules_) {
        if (rule.pattern.empty()) {
            continue;
        }
        if (!matches_pattern(rule.pattern, path)) {
            continue;
        }
        if (best == nullptr || pattern_length(rule.pattern) > pattern_length(best->pattern) ||
            (pattern_length(rule.pattern) == pattern_length(best->pattern) && rule.allow)) {
            best = &rule;
        }
    }

    return best == nullptr || best->allow;
}
