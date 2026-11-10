#include "analyzer.hpp"

#include <string>
#include <utility>

#include "normalize.hpp"
#include "stemmer.hpp"
#include "stopwords.hpp"

std::vector<Token> stem_tokens(std::vector<Token> tokens) {
    for (Token& token : tokens) {
        token.text = porter::stem(token.text);
    }

    return tokens;
}

namespace {

std::size_t separator_length(std::string_view text, std::size_t at) {
    const auto byte = [&](std::size_t i) { return static_cast<unsigned char>(text[i]); };
    if (text[at] == '-' || text[at] == '/') {
        return 1;
    }
    if (at + 2 < text.size() && byte(at) == 0xE2 && byte(at + 1) == 0x80 && byte(at + 2) >= 0x90 &&
        byte(at + 2) <= 0x95) {
        return 3;
    }
    return 0;
}

}

std::vector<Token> split_compounds(std::vector<Token> tokens) {
    std::vector<Token> out;
    out.reserve(tokens.size());
    std::size_t ordinal = 0;

    for (Token& token : tokens) {
        const std::string_view text = token.text;
        std::size_t start = 0;
        std::size_t at = 0;
        bool split = false;
        while (at < text.size()) {
            const std::size_t length = separator_length(text, at);
            if (length == 0) {
                at++;
                continue;
            }
            split = true;
            if (at > start) {
                out.push_back(Token{std::string(text.substr(start, at - start)), ordinal++,
                                    token.offset + start});
            }
            at += length;
            start = at;
        }

        if (!split) {
            token.position = ordinal++;
            out.push_back(std::move(token));
        } else if (start < text.size()) {
            out.push_back(
                Token{std::string(text.substr(start)), ordinal++, token.offset + start});
        }
    }

    return out;
}

std::vector<Token> analyze(std::string_view text) {
    return stem_tokens(remove_stopwords(normalize_tokens(split_compounds(tokenize(text)))));
}

std::vector<Token> analyze_document(const Document& doc) {
    return analyze(doc.title + "\n\n" + doc.body);
}

std::size_t title_span(const Document& doc) {
    return split_compounds(tokenize(doc.title)).size();
}
