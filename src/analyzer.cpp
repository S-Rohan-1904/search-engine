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

std::vector<Token> analyze(std::string_view text) {
    return stem_tokens(remove_stopwords(normalize_tokens(tokenize(text))));
}

std::vector<Token> analyze_document(const Document& doc) {
    return analyze(doc.title + "\n\n" + doc.body);
}
