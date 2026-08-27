#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "tokenizer.hpp"

// Reduces a raw token to the term that gets indexed. Returns an empty string
// when nothing indexable survives, meaning the token should be dropped.
//
// Kept: ASCII letters (folded to lowercase), ASCII digits, any byte >= 0x80
// verbatim so UTF-8 sequences survive, a hyphen with a keeper on both sides so
// "state-of-the-art" stays one term, and a period between two digits so "3.14"
// keeps its point. Everything else is dropped.
//
// Two consequences worth remembering: apostrophes vanish, so "don't" becomes
// "dont" and the possessive "cats'" collides with the plural "cats"; and the
// periods in an acronym sit between letters rather than digits, so "U.S.A."
// collapses to "usa".
//
// Neighbour tests look at the original token rather than at what has been
// emitted so far, so "a--b" drops both hyphens and yields "ab".
std::string normalize(std::string_view token);

// Normalizes a token stream, dropping the tokens that normalize away.
// Survivors keep their original position and offset, so a drop shows up as a
// gap rather than renumbering what follows.
std::vector<Token> normalize_tokens(std::vector<Token> tokens);
