#pragma once

#include <string_view>
#include <vector>

#include "tokenizer.hpp"

// True if term is a stopword. The comparison is exact, so term must already be
// normalized: "The" and "don't" will not match as they stand.
bool is_stopword(std::string_view term);

// Drops every stopword from a term stream. As in normalize_tokens, survivors
// keep their original position and offset.
std::vector<Token> remove_stopwords(std::vector<Token> tokens);
