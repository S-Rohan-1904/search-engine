#pragma once

#include <filesystem>
#include <memory>
#include <optional>
#include <string>

#include "url.hpp"

// What a fetch returned.
struct FetchResult {
    int status;                // 200, 404, and so on
    std::string content_type;  // lowercased, parameters stripped
    std::string body;
};

// Where a crawler gets its bytes.
//
// The crawler depends on this interface rather than on HTTP, which is what
// makes it testable: the tests point it at a directory of files on disk and get
// a byte-for-byte reproducible crawl with no network involved.
class Fetcher {
public:
    virtual ~Fetcher() = default;
    virtual std::optional<FetchResult> get(const Url& url) = 0;
};

// Serves a mirrored site from the filesystem.
//
// A URL maps to `root/<host>/<path>`, with a directory or a trailing slash
// resolving to `index.html` inside it. A missing file is a 404 rather than an
// error, so the crawler exercises the same path it would against a real server.
std::unique_ptr<Fetcher> make_file_fetcher(std::filesystem::path root);

// Fetches over HTTP with libcurl. Returns nullptr when the build has no curl.
std::unique_ptr<Fetcher> make_http_fetcher(std::string user_agent, long timeout_seconds);
