#pragma once

#include <optional>
#include <string>
#include <string_view>

// Reading a couple of fields out of a JSON Lines record.
//
// Scanned rather than parsed: the files this reads carry twenty fields and two
// are wanted, and a JSON parser would cost a dependency for that. Two parts of
// the shortcut are not optional, because getting either wrong corrupts text
// silently rather than failing:
//
//   * A key match must check that the key really is one. Searching for "text":"
//     finds it inside "opening_text":" and "auxiliary_text":" as well, so the
//     byte before the opening quote has to be '{' or ','.
//   * String unescaping must be complete: the six short escapes, the escaped
//     quote, slash and backslash, and \uXXXX including surrogate pairs, which
//     are how JSON carries anything outside the basic multilingual plane.

// The value of `key` in `line`, unescaped, or nullopt when the key is absent.
// Only string values are read; a number or object value returns nullopt.
std::optional<std::string> field_of(std::string_view line, std::string_view key);

// Runs of whitespace squeezed to one space, and the ends trimmed.
std::string collapse(std::string_view text);
