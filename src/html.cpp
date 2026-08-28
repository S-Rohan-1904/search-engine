#include "html.hpp"

#include <cctype>

namespace {

bool starts_with_ci(std::string_view text, std::size_t position, std::string_view prefix) {
    if (text.size() - position < prefix.size()) {
        return false;
    }
    for (std::size_t i = 0; i < prefix.size(); i++) {
        const char a = static_cast<char>(std::tolower(static_cast<unsigned char>(text[position + i])));
        const char b = static_cast<char>(std::tolower(static_cast<unsigned char>(prefix[i])));
        if (a != b) {
            return false;
        }
    }
    return true;
}

std::size_t skip_element(std::string_view html, std::size_t position, std::string_view closing) {
    const std::size_t end = html.size();
    for (std::size_t i = position; i + closing.size() <= end; i++) {
        if (starts_with_ci(html, i, closing)) {
            const std::size_t close = html.find('>', i);
            return close == std::string_view::npos ? end : close + 1;
        }
    }
    return end;
}

void append_entity(std::string& out, std::string_view name) {
    if (name == "amp") {
        out.push_back('&');
    } else if (name == "lt") {
        out.push_back('<');
    } else if (name == "gt") {
        out.push_back('>');
    } else if (name == "quot") {
        out.push_back('"');
    } else if (name == "apos" || name == "#39") {
        out.push_back('\'');
    } else if (name == "nbsp") {
        out.push_back(' ');
    } else if (!name.empty() && name.front() == '#') {
        unsigned long code = 0;
        const bool hex = name.size() > 1 && (name[1] == 'x' || name[1] == 'X');
        for (std::size_t i = hex ? 2 : 1; i < name.size(); i++) {
            const unsigned char c = static_cast<unsigned char>(name[i]);
            const int digit = std::isdigit(c) ? c - '0'
                              : hex && std::isxdigit(c)
                                  ? std::tolower(c) - 'a' + 10
                                  : -1;
            if (digit < 0) {
                out += "&";
                out += name;
                out += ";";
                return;
            }
            code = code * (hex ? 16u : 10u) + static_cast<unsigned long>(digit);
        }
        if (code < 0x80) {
            out.push_back(static_cast<char>(code));
        } else if (code < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (code >> 6)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (code >> 12)));
            out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
        }
    } else {
        out += "&";
        out += name;
        out += ";";
    }
}

std::string decode_entities(std::string_view text) {
    std::string out;
    out.reserve(text.size());

    for (std::size_t i = 0; i < text.size(); i++) {
        if (text[i] != '&') {
            out.push_back(text[i]);
            continue;
        }

        const std::size_t semicolon = text.find(';', i + 1);
        if (semicolon == std::string_view::npos || semicolon - i > 12) {
            out.push_back('&');
            continue;
        }

        append_entity(out, text.substr(i + 1, semicolon - i - 1));
        i = semicolon;
    }

    return out;
}

std::string collapse_whitespace(std::string_view text) {
    std::string out;
    out.reserve(text.size());

    bool pending_space = false;
    for (const char c : text) {
        if (std::isspace(static_cast<unsigned char>(c)) != 0) {
            pending_space = !out.empty();
            continue;
        }
        if (pending_space) {
            out.push_back(' ');
            pending_space = false;
        }
        out.push_back(c);
    }

    return out;
}

std::string attribute_value(std::string_view tag, std::string_view name) {
    for (std::size_t i = 0; i + name.size() < tag.size(); i++) {
        if (!starts_with_ci(tag, i, name)) {
            continue;
        }
        if (i > 0 && std::isalnum(static_cast<unsigned char>(tag[i - 1])) != 0) {
            continue;
        }

        std::size_t j = i + name.size();
        while (j < tag.size() && std::isspace(static_cast<unsigned char>(tag[j])) != 0) {
            j++;
        }
        if (j >= tag.size() || tag[j] != '=') {
            continue;
        }
        j++;
        while (j < tag.size() && std::isspace(static_cast<unsigned char>(tag[j])) != 0) {
            j++;
        }
        if (j >= tag.size()) {
            return {};
        }

        if (tag[j] == '"' || tag[j] == '\'') {
            const char quote = tag[j];
            const std::size_t end = tag.find(quote, j + 1);
            if (end == std::string_view::npos) {
                return {};
            }
            return decode_entities(tag.substr(j + 1, end - j - 1));
        }

        std::size_t end = j;
        while (end < tag.size() && std::isspace(static_cast<unsigned char>(tag[end])) == 0) {
            end++;
        }
        return decode_entities(tag.substr(j, end - j));
    }

    return {};
}

bool is_block_tag(std::string_view name) {
    static constexpr std::string_view kBlockTags[] = {
        "p",  "div",  "br", "li", "ul", "ol", "tr", "td", "th", "h1", "h2",
        "h3", "h4",   "h5", "h6", "section", "article", "header", "footer",
        "nav", "aside", "table", "blockquote", "pre", "hr", "body", "title"};

    for (const std::string_view tag : kBlockTags) {
        if (name == tag) {
            return true;
        }
    }
    return false;
}

}

HtmlPage parse_html(std::string_view html) {
    HtmlPage page;
    std::string raw_text;
    std::string raw_title;
    bool in_title = false;

    std::size_t i = 0;
    while (i < html.size()) {
        if (html[i] != '<') {
            if (in_title) {
                raw_title.push_back(html[i]);
            }
            raw_text.push_back(html[i]);
            i++;
            continue;
        }

        if (starts_with_ci(html, i, "<!--")) {
            const std::size_t end = html.find("-->", i + 4);
            i = end == std::string_view::npos ? html.size() : end + 3;
            continue;
        }

        if (starts_with_ci(html, i, "<script")) {
            i = skip_element(html, i, "</script");
            raw_text.push_back(' ');
            continue;
        }

        if (starts_with_ci(html, i, "<style")) {
            i = skip_element(html, i, "</style");
            raw_text.push_back(' ');
            continue;
        }

        const std::size_t close = html.find('>', i);
        if (close == std::string_view::npos) {
            break;
        }

        const std::string_view tag = html.substr(i + 1, close - i - 1);

        std::size_t name_start = 0;
        if (name_start < tag.size() && tag[name_start] == '/') {
            name_start++;
        }
        std::size_t name_end = name_start;
        while (name_end < tag.size() &&
               std::isalnum(static_cast<unsigned char>(tag[name_end])) != 0) {
            name_end++;
        }

        std::string name;
        for (std::size_t k = name_start; k < name_end; k++) {
            name.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(tag[k]))));
        }

        if (name == "title") {
            in_title = !tag.empty() && tag.front() != '/';
        } else if (name == "a" && !tag.empty() && tag.front() != '/') {
            std::string href = attribute_value(tag, "href");
            if (!href.empty()) {
                page.links.push_back(std::move(href));
            }
        }

        if (is_block_tag(name)) {
            raw_text.push_back('\n');
        }

        i = close + 1;
    }

    page.title = collapse_whitespace(decode_entities(raw_title));
    page.text = collapse_whitespace(decode_entities(raw_text));
    return page;
}
