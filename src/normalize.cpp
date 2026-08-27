#include "normalize.hpp"

#include <utility>

namespace {

bool is_ascii_digit(unsigned char c) {
    return c >= '0' && c <= '9';
}

bool is_ascii_alpha(unsigned char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_wordish(unsigned char c) {
    return is_ascii_alpha(c) || is_ascii_digit(c) || c >= 0x80;
}

char fold(unsigned char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }
    return static_cast<char>(c);
}

}

std::string normalize(std::string_view token) {
    std::string out;
    out.reserve(token.size());

    for (std::size_t index = 0; index < token.size(); ++index) {
        const unsigned char ch = static_cast<unsigned char>(token[index]);

        if (ch == '\'') {
            continue;
        }
        if (is_wordish(ch)) {
            out.push_back(fold(ch));
            continue;
        }

        if (index == 0 || index + 1 == token.size()) {
            continue;
        }

        const unsigned char prev = static_cast<unsigned char>(token[index - 1]);
        const unsigned char next = static_cast<unsigned char>(token[index + 1]);

        if (ch == '-' && is_wordish(prev) && is_wordish(next)) {
            out.push_back('-');
        } else if (ch == '.' && is_ascii_digit(prev) && is_ascii_digit(next)) {
            out.push_back('.');
        }
    }

    return out;
}

std::vector<Token> normalize_tokens(std::vector<Token> tokens) {
    std::vector<Token> out;
    out.reserve(tokens.size());

    for (Token& token : tokens) {
        std::string term = normalize(token.text);
        if (term.empty()) {
            continue;
        }
        token.text = std::move(term);
        out.push_back(std::move(token));
    }

    return out;
}
