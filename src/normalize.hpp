#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "tokenizer.hpp"

// Reduces a raw token to the term that gets indexed. Returns an empty string
// when nothing indexable survives, meaning the token should be dropped.
//
// Kept: ASCII letters (folded to lowercase), ASCII digits, a hyphen with a
// keeper on both sides so "state-of-the-art" stays one term, and a period
// between two digits so "3.14" keeps its point. Everything else is dropped.
//
// Non-ASCII text is decoded as UTF-8 and folded where an ASCII equivalent
// exists, covering Latin-1 Supplement and Latin Extended-A:
//
//     Café    -> cafe        Straße  -> strasse
//     Zürich  -> zurich      Œuvre   -> oeuvre
//
// So an accented word and its unaccented spelling produce the same term, which
// is what a searcher typing "cafe" expects of "Café". The folding is by
// transliteration rather than Unicode case folding: real case folding needs
// ICU-sized tables, and for a Latin-script corpus this reaches nearly the same
// place for a table of 192 entries.
//
// Anything outside that range is kept verbatim, so Chinese, Cyrillic and Greek
// survive as their own terms rather than being mangled into ASCII. A byte
// sequence that is not valid UTF-8 is kept as it is rather than dropped.
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
