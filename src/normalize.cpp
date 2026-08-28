#include "normalize.hpp"

#include <cstddef>
#include <string_view>
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

constexpr char32_t kFoldFirst = 0xC0;
constexpr char32_t kFoldLast = 0x17F;

constexpr std::string_view kFoldTable[] = {
    "a", "a", "a", "a", "a", "a", "ae", "c",
    "e", "e", "e", "e", "i", "i", "i", "i",
    "d", "n", "o", "o", "o", "o", "o", "",
    "o", "u", "u", "u", "u", "y", "th", "ss",
    "a", "a", "a", "a", "a", "a", "ae", "c",
    "e", "e", "e", "e", "i", "i", "i", "i",
    "d", "n", "o", "o", "o", "o", "o", "",
    "o", "u", "u", "u", "u", "y", "th", "y",
    "a", "a", "a", "a", "a", "a", "c", "c",
    "c", "c", "c", "c", "c", "c", "d", "d",
    "d", "d", "e", "e", "e", "e", "e", "e",
    "e", "e", "e", "e", "g", "g", "g", "g",
    "g", "g", "g", "g", "h", "h", "h", "h",
    "i", "i", "i", "i", "i", "i", "i", "i",
    "i", "i", "ij", "ij", "j", "j", "k", "k",
    "k", "l", "l", "l", "l", "l", "l", "l",
    "l", "l", "l", "n", "n", "n", "n", "n",
    "n", "n", "n", "n", "o", "o", "o", "o",
    "o", "o", "oe", "oe", "r", "r", "r", "r",
    "r", "r", "s", "s", "s", "s", "s", "s",
    "s", "s", "t", "t", "t", "t", "t", "t",
    "u", "u", "u", "u", "u", "u", "u", "u",
    "u", "u", "u", "u", "w", "w", "y", "y",
    "y", "z", "z", "z", "z", "z", "z", "s",
};

std::size_t utf8_length(unsigned char lead) {
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 0;
}

bool decode_utf8(std::string_view text, std::size_t at, char32_t& code, std::size_t& length) {
    const unsigned char lead = static_cast<unsigned char>(text[at]);
    length = utf8_length(lead);
    if (length == 0 || text.size() - at < length) {
        return false;
    }

    static constexpr unsigned char kLeadMask[] = {0, 0, 0x1F, 0x0F, 0x07};
    code = lead & kLeadMask[length];

    for (std::size_t i = 1; i < length; i++) {
        const unsigned char next = static_cast<unsigned char>(text[at + i]);
        if ((next & 0xC0) != 0x80) {
            return false;
        }
        code = (code << 6) | (next & 0x3F);
    }
    return true;
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
        if (ch >= 0x80) {
            char32_t code = 0;
            std::size_t length = 0;
            if (decode_utf8(token, index, code, length)) {
                if (code >= kFoldFirst && code <= kFoldLast) {
                    out.append(kFoldTable[code - kFoldFirst]);
                } else {
                    out.append(token.substr(index, length));
                }
                index += length - 1;
            } else {
                out.push_back(static_cast<char>(ch));
            }
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
