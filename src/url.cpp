#include "url.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace {

std::string lowercase(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

int default_port(std::string_view scheme) {
    return scheme == "https" ? 443 : 80;
}

std::string normalize_path(std::string_view path) {
    std::vector<std::string_view> segments;
    std::size_t i = 0;

    while (i < path.size()) {
        const std::size_t next = path.find('/', i);
        const std::string_view segment =
            path.substr(i, next == std::string_view::npos ? std::string_view::npos : next - i);

        if (segment == "..") {
            if (!segments.empty()) {
                segments.pop_back();
            }
        } else if (!segment.empty() && segment != ".") {
            segments.push_back(segment);
        }

        if (next == std::string_view::npos) {
            break;
        }
        i = next + 1;
    }

    std::string out;
    for (const std::string_view segment : segments) {
        out.push_back('/');
        out.append(segment);
    }

    const bool trailing_slash = !path.empty() && path.back() == '/';
    if (out.empty()) {
        return "/";
    }
    if (trailing_slash) {
        out.push_back('/');
    }
    return out;
}

}

std::optional<Url> parse_url(std::string_view text) {
    const std::size_t scheme_end = text.find("://");
    if (scheme_end == std::string_view::npos) {
        return std::nullopt;
    }

    Url url;
    url.scheme = lowercase(text.substr(0, scheme_end));
    if (url.scheme != "http" && url.scheme != "https") {
        return std::nullopt;
    }

    std::string_view rest = text.substr(scheme_end + 3);

    const std::size_t fragment = rest.find('#');
    if (fragment != std::string_view::npos) {
        rest = rest.substr(0, fragment);
    }

    std::string_view query;
    const std::size_t question = rest.find('?');
    if (question != std::string_view::npos) {
        query = rest.substr(question + 1);
        rest = rest.substr(0, question);
    }

    std::string_view path;
    const std::size_t slash = rest.find('/');
    if (slash != std::string_view::npos) {
        path = rest.substr(slash);
        rest = rest.substr(0, slash);
    }

    std::string_view authority = rest;
    url.port = default_port(url.scheme);
    const std::size_t colon = authority.find(':');
    if (colon != std::string_view::npos) {
        const std::string_view port_text = authority.substr(colon + 1);
        authority = authority.substr(0, colon);

        int port = 0;
        for (const char c : port_text) {
            if (std::isdigit(static_cast<unsigned char>(c)) == 0) {
                return std::nullopt;
            }
            port = port * 10 + (c - '0');
            if (port > 65535) {
                return std::nullopt;
            }
        }
        if (port_text.empty()) {
            return std::nullopt;
        }
        url.port = port;
    }

    if (authority.empty()) {
        return std::nullopt;
    }

    url.host = lowercase(authority);
    url.path = normalize_path(path);
    url.query = std::string(query);
    return url;
}

std::optional<Url> resolve_url(const Url& base, std::string_view reference) {
    while (!reference.empty() && (reference.front() == ' ' || reference.front() == '\t' ||
                                  reference.front() == '\n' || reference.front() == '\r')) {
        reference.remove_prefix(1);
    }
    while (!reference.empty() && (reference.back() == ' ' || reference.back() == '\t' ||
                                 reference.back() == '\n' || reference.back() == '\r')) {
        reference.remove_suffix(1);
    }

    if (reference.empty()) {
        return std::nullopt;
    }

    if (reference.find("://") != std::string_view::npos) {
        return parse_url(reference);
    }

    if (reference.starts_with("//")) {
        return parse_url(base.scheme + ":" + std::string(reference));
    }

    const std::size_t colon = reference.find(':');
    const std::size_t slash = reference.find('/');
    if (colon != std::string_view::npos && (slash == std::string_view::npos || colon < slash)) {
        return std::nullopt;
    }

    if (reference.front() == '#') {
        return std::nullopt;
    }

    std::string prefix = base.scheme + "://" + base.host;
    if (base.port != default_port(base.scheme)) {
        prefix += ":" + std::to_string(base.port);
    }

    if (reference.front() == '/') {
        return parse_url(prefix + std::string(reference));
    }

    std::string directory = base.path;
    const std::size_t last_slash = directory.rfind('/');
    directory.resize(last_slash == std::string::npos ? 0 : last_slash + 1);

    return parse_url(prefix + directory + std::string(reference));
}

std::string url_to_string(const Url& url) {
    std::string out = url.scheme + "://" + url.host;
    if (url.port != default_port(url.scheme)) {
        out += ":" + std::to_string(url.port);
    }
    out += url.path;
    if (!url.query.empty()) {
        out += "?" + url.query;
    }
    return out;
}
