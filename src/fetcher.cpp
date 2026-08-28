#include "fetcher.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <utility>

#ifdef SEARCH_HAVE_CURL
#include <curl/curl.h>
#endif

namespace {

std::string extension_content_type(const std::filesystem::path& path) {
    const std::string extension = path.extension().string();
    if (extension == ".html" || extension == ".htm") {
        return "text/html";
    }
    if (extension == ".txt") {
        return "text/plain";
    }
    return "application/octet-stream";
}

class FileFetcher : public Fetcher {
public:
    explicit FileFetcher(std::filesystem::path root) : root_(std::move(root)) {}

    std::optional<FetchResult> get(const Url& url) override {
        std::filesystem::path path = root_ / url.host;
        std::string relative = url.path;
        if (!relative.empty() && relative.front() == '/') {
            relative.erase(relative.begin());
        }
        if (!relative.empty()) {
            path /= relative;
        }

        std::error_code ec;
        if (std::filesystem::is_directory(path, ec) || url.path.back() == '/') {
            path /= "index.html";
        }

        std::ifstream in(path, std::ios::binary);
        if (!in) {
            return FetchResult{404, "", ""};
        }

        std::string body((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        return FetchResult{200, extension_content_type(path), std::move(body)};
    }

private:
    std::filesystem::path root_;
};

#ifdef SEARCH_HAVE_CURL

std::size_t append_to_string(char* data, std::size_t size, std::size_t count, void* user) {
    const std::size_t total = size * count;
    static_cast<std::string*>(user)->append(data, total);
    return total;
}

class CurlFetcher : public Fetcher {
public:
    CurlFetcher(std::string user_agent, long timeout_seconds)
        : user_agent_(std::move(user_agent)), timeout_seconds_(timeout_seconds) {
        handle_ = curl_easy_init();
    }

    ~CurlFetcher() override {
        if (handle_ != nullptr) {
            curl_easy_cleanup(handle_);
        }
    }

    std::optional<FetchResult> get(const Url& url) override {
        if (handle_ == nullptr) {
            return std::nullopt;
        }

        std::string body;
        const std::string text = url_to_string(url);

        curl_easy_reset(handle_);
        curl_easy_setopt(handle_, CURLOPT_URL, text.c_str());
        curl_easy_setopt(handle_, CURLOPT_USERAGENT, user_agent_.c_str());
        curl_easy_setopt(handle_, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(handle_, CURLOPT_MAXREDIRS, 5L);
        curl_easy_setopt(handle_, CURLOPT_TIMEOUT, timeout_seconds_);
        curl_easy_setopt(handle_, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(handle_, CURLOPT_WRITEFUNCTION, append_to_string);
        curl_easy_setopt(handle_, CURLOPT_WRITEDATA, &body);

        if (curl_easy_perform(handle_) != CURLE_OK) {
            return std::nullopt;
        }

        long status = 0;
        curl_easy_getinfo(handle_, CURLINFO_RESPONSE_CODE, &status);

        char* content_type = nullptr;
        curl_easy_getinfo(handle_, CURLINFO_CONTENT_TYPE, &content_type);

        std::string type = content_type == nullptr ? "" : content_type;
        const std::size_t semicolon = type.find(';');
        if (semicolon != std::string::npos) {
            type.resize(semicolon);
        }
        std::transform(type.begin(), type.end(), type.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });

        return FetchResult{static_cast<int>(status), type, std::move(body)};
    }

private:
    CURL* handle_ = nullptr;
    std::string user_agent_;
    long timeout_seconds_;
};

#endif

}

std::unique_ptr<Fetcher> make_file_fetcher(std::filesystem::path root) {
    return std::make_unique<FileFetcher>(std::move(root));
}

std::unique_ptr<Fetcher> make_http_fetcher(std::string user_agent, long timeout_seconds) {
#ifdef SEARCH_HAVE_CURL
    return std::make_unique<CurlFetcher>(std::move(user_agent), timeout_seconds);
#else
    (void)user_agent;
    (void)timeout_seconds;
    return nullptr;
#endif
}
