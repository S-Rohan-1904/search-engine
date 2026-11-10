#include "http.hpp"

#include <cstdio>
#include <sstream>

namespace {

int hex_value(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

const char* reason(int status) {
    switch (status) {
        case 200:
            return "OK";
        case 400:
            return "Bad Request";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        default:
            return "Internal Server Error";
    }
}

}

std::string url_decode(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); i++) {
        if (text[i] == '+') {
            out.push_back(' ');
        } else if (text[i] == '%' && i + 2 < text.size() && hex_value(text[i + 1]) >= 0 &&
                   hex_value(text[i + 2]) >= 0) {
            out.push_back(static_cast<char>(hex_value(text[i + 1]) * 16 + hex_value(text[i + 2])));
            i += 2;
        } else {
            out.push_back(text[i]);
        }
    }
    return out;
}

std::string url_encode(std::string_view text) {
    static const char* const kHex = "0123456789ABCDEF";
    std::string out;
    for (const char c : text) {
        const auto byte = static_cast<unsigned char>(c);
        if ((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
            (byte >= '0' && byte <= '9') || byte == '-' || byte == '_' || byte == '.' ||
            byte == '~') {
            out.push_back(c);
        } else if (byte == ' ') {
            out.push_back('+');
        } else {
            out.push_back('%');
            out.push_back(kHex[byte >> 4]);
            out.push_back(kHex[byte & 0x0F]);
        }
    }
    return out;
}

std::optional<HttpRequest> parse_request(std::string_view raw) {
    const std::size_t line_end = raw.find("\r\n");
    const std::string_view line = raw.substr(0, line_end == std::string_view::npos ? raw.find('\n')
                                                                                    : line_end);

    const std::size_t first = line.find(' ');
    if (first == std::string_view::npos || first == 0) {
        return std::nullopt;
    }
    const std::size_t second = line.find(' ', first + 1);
    if (second == std::string_view::npos || second == first + 1) {
        return std::nullopt;
    }
    if (!line.substr(second + 1).starts_with("HTTP/1.")) {
        return std::nullopt;
    }

    HttpRequest request;
    request.method = std::string(line.substr(0, first));
    const std::string_view target = line.substr(first + 1, second - first - 1);
    if (!target.starts_with('/')) {
        return std::nullopt;
    }

    const std::size_t question = target.find('?');
    request.path = url_decode(target.substr(0, question));
    if (question == std::string_view::npos) {
        return request;
    }

    std::string_view rest = target.substr(question + 1);
    while (!rest.empty()) {
        const std::size_t amp = rest.find('&');
        const std::string_view pair = rest.substr(0, amp);
        const std::size_t equals = pair.find('=');
        const std::string key = url_decode(pair.substr(0, equals));
        if (!key.empty() && request.query.count(key) == 0) {
            request.query.emplace(key, equals == std::string_view::npos
                                           ? std::string()
                                           : url_decode(pair.substr(equals + 1)));
        }
        if (amp == std::string_view::npos) {
            break;
        }
        rest = rest.substr(amp + 1);
    }
    return request;
}

std::string html_escape(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (const char c : text) {
        switch (c) {
            case '&':
                out += "&amp;";
                break;
            case '<':
                out += "&lt;";
                break;
            case '>':
                out += "&gt;";
                break;
            case '"':
                out += "&quot;";
                break;
            case '\'':
                out += "&#39;";
                break;
            default:
                out.push_back(c);
        }
    }
    return out;
}

std::string json_escape(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 2);
    for (const char c : text) {
        const auto byte = static_cast<unsigned char>(c);
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (byte < 0x20) {
                    char buffer[8];
                    std::snprintf(buffer, sizeof buffer, "\\u%04x", byte);
                    out += buffer;
                } else {
                    out.push_back(c);
                }
        }
    }
    return out;
}

std::string serialize(const HttpResponse& response) {
    std::ostringstream out;
    out << "HTTP/1.1 " << response.status << " " << reason(response.status) << "\r\n"
        << "Content-Type: " << response.content_type << "\r\n"
        << "Content-Length: " << response.body.size() << "\r\n"
        << "Connection: close\r\n"
        << "\r\n"
        << response.body;
    return out.str();
}
