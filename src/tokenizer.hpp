#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

struct Token {
    std::string text;      // as it appeared, not yet normalized
    std::size_t position;  // 0-based ordinal in the token stream
    std::size_t offset;    // byte offset into the source text
};

// Splits text on ASCII whitespace. A token is a maximal run of non-whitespace
// bytes; runs of whitespace of any length separate tokens, and leading or
// trailing whitespace produces none.
//
// Nothing is normalized here, so "THE-END," comes back as a single token
// spelled exactly that way. Offsets are byte indices, not character indices,
// which is what snippet extraction needs to slice UTF-8 text back out.
std::vector<Token> tokenize(std::string_view text);
