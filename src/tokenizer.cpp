#include "tokenizer.hpp"

#include <string>
#include <utility>

namespace {
bool is_space(unsigned char c) {
    switch (c) {
        case ' ':
        case '\n':
        case '\r':
        case '\t':
        case '\f':
        case '\v':
            return true;
        default:
            return false;
    }
}

}

std::vector<Token> tokenize(std::string_view text) {
    std::vector<Token> tokens;
    std::size_t index = 0;
    std::size_t ordinal = 0;

    while (index < text.size()) {
        while (index < text.size() && is_space(static_cast<unsigned char>(text[index]))) {
            ++index;
        }
        if (index == text.size()) {
            break;
        }

        const std::size_t start_offset = index;
        while (index < text.size() && !is_space(static_cast<unsigned char>(text[index]))) {
            ++index;
        }

        std::string token_text{text.substr(start_offset, index - start_offset)};
        tokens.push_back(Token{std::move(token_text), ordinal, start_offset});
        ++ordinal;
    }

    return tokens;
}
