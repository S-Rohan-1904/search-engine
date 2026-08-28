#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

// How to build a snippet.
struct SnippetOptions {
    std::size_t max_chars = 200;
    std::string open = "[";
    std::string close = "]";
};

// A short excerpt of `text` around the query terms, with the matches marked.
//
// `terms` are analyzed query terms, as the index stores them. The document's
// own words are analyzed the same way to find matches, so a search for "cats"
// highlights the word "Cat" where it appears, spelled as the document spells
// it.
//
// This is why every token has carried a byte offset since the tokenizer: the
// snippet is cut out of the original text, not reassembled from indexed terms,
// so the reader sees the document's own capitalization, punctuation and
// stopwords.
//
// The window chosen is the one covering the most distinct query terms, then
// the most matches, then the earliest. A document with no match at all yields
// its opening, which is what a search result should show when the match was in
// the title.
std::string make_snippet(std::string_view text, const std::vector<std::string>& terms,
                         const SnippetOptions& options);
