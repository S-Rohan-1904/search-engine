#include "document.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr std::string_view kWhitespace = " \t\r\n\f\v";

std::string_view trim(std::string_view line) {
    const auto start = line.find_first_not_of(kWhitespace);
    if (start == std::string_view::npos) {
        return {};
    }

    const auto end = line.find_last_not_of(kWhitespace);
    return line.substr(start, end - start + 1);
}

bool is_blank(std::string_view line) {
    return trim(line).empty();
}


std::vector<std::string> split_lines(std::string_view raw) {
    std::istringstream stream{std::string(raw)};
    std::vector<std::string> lines;
    std::string line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

}

std::optional<Document> parse_document(std::string id, std::string_view raw) {
    if (is_blank(raw)) {
        return std::nullopt;
    }

    const std::vector<std::string> lines = split_lines(raw);

    const std::string_view header = lines[0];
    const auto colon = header.find(':');
    if (colon == std::string_view::npos) {
        return std::nullopt;
    }
    if (trim(header.substr(0, colon)) != "title") {
        return std::nullopt;
    }
    std::string title{trim(header.substr(colon + 1))};

    std::size_t first = 1;
    std::size_t last = lines.size();
    while (first < last && is_blank(lines[first])) {
        ++first;
    }
    while (last > first && is_blank(lines[last - 1])) {
        --last;
    }

    std::string body;
    for (std::size_t i = first; i < last; ++i) {
        if (i > first) {
            body += '\n';
        }
        body += lines[i];
    }

    return Document{std::move(id), std::move(title), std::move(body)};
}

std::optional<Document> read_document(const std::filesystem::path& corpus_dir, const std::string& id) {
    std::ifstream file(corpus_dir / (id + ".txt"), std::ios::binary);
    
    if (!file) {
        return std::nullopt;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return parse_document(id, buffer.str());
}
