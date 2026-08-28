#pragma once

#include <cstddef>
#include <string_view>

// Levenshtein distance: the fewest single-character insertions, deletions and
// substitutions that turn `a` into `b`.
//
// Computed with two rows of a dynamic programming table rather than the whole
// grid. The recurrence only ever reads the row above and the cell to the left,
// so the memory is O(min(|a|, |b|)) rather than O(|a| * |b|), and the rows stay
// in cache for the word lengths this is used on.
std::size_t edit_distance(std::string_view a, std::string_view b);

// The same distance, abandoned once it is known to exceed `limit`.
//
// Returns limit + 1 to mean "further than that". Spelling correction asks this
// question thousands of times per query and discards almost every answer, so
// the useful optimisation is not computing a number nobody reads: if every cell
// of a row exceeds the limit, no later row can come back under it.
std::size_t bounded_edit_distance(std::string_view a, std::string_view b, std::size_t limit);
