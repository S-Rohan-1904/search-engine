#pragma once

#include <optional>
#include <string>
#include <string_view>

// An absolute URL, split into the parts a crawler needs.
struct Url {
    std::string scheme;  // "http" or "https", lowercased
    std::string host;    // lowercased, no port
    int port;            // the scheme's default when none was given
    std::string path;    // always begins with '/'
    std::string query;   // without the '?', empty when absent
};

// Parses an absolute URL. Returns nullopt for anything without a scheme and
// host, or with a scheme other than http or https.
//
// Normalization happens here so that two spellings of the same page compare
// equal: the scheme and host are lowercased, a default port is dropped, an
// empty path becomes "/", "." and ".." segments are resolved, and the fragment
// is discarded. A fragment names a place within a page, not a different page,
// so a crawler that kept it would fetch the same document repeatedly.
std::optional<Url> parse_url(std::string_view text);

// Resolves a possibly relative reference against a base URL, the way a browser
// resolves an href.
//
// Handles absolute URLs, protocol-relative ("//host/path"), root-relative
// ("/path"), and relative ("path", "../path") references. Returns nullopt if
// the result would not be an http or https URL, which is how "mailto:" and
// "javascript:" links get dropped.
std::optional<Url> resolve_url(const Url& base, std::string_view reference);

// The canonical text of a URL. Two URLs that name the same page produce the
// same string, which is what the frontier deduplicates on.
std::string url_to_string(const Url& url);
