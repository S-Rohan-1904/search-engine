#pragma once

#include <string_view>
#include <vector>

#include "document.hpp"
#include "tokenizer.hpp"

// Replaces each token's text with its Porter stem. Tokens must already be
// normalized, since the stemmer only handles lowercase ASCII. Nothing is
// dropped and positions are untouched.
std::vector<Token> stem_tokens(std::vector<Token> tokens);

// The full chain: tokenize, normalize, drop stopwords, stem.
//
// The order matters. Normalization comes first because the stopword list is
// stored normalized. Stopword removal comes before stemming because the list
// holds words rather than stems, so filtering afterwards would need a stemmed
// list to compare against.
//
// Positions and offsets come from the original tokenization and survive every
// filter, so position 4 still means the fifth token of the source text however
// much was removed before it. Phrase matching depends on those gaps.
std::vector<Token> analyze(std::string_view text);

// Analyzes a document as one stream: title, blank line, body. Title terms come
// first and body offsets are shifted past the title.
std::vector<Token> analyze_document(const Document& doc);
