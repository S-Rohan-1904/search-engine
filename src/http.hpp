#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>

struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> query;
};

struct HttpResponse {
    int status = 200;
    std::string content_type = "text/html; charset=utf-8";
    std::string body;
};

std::optional<HttpRequest> parse_request(std::string_view raw);

std::string url_decode(std::string_view text);

std::string url_encode(std::string_view text);

std::string html_escape(std::string_view text);

std::string json_escape(std::string_view text);

std::string serialize(const HttpResponse& response);
