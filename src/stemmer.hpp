#pragma once

#include <cstddef>
#include <string>
#include <string_view>

// The Porter stemming algorithm, as described in
// https://tartarus.org/martin/PorterStemmer/def.txt
//
// Everything here works on raw bytes. Anything that is not one of the five
// vowels counts as a consonant, digits and UTF-8 bytes included, so no input
// needs rejecting.
namespace porter {

// True if the byte at index is a consonant. A 'y' is a consonant when it opens
// the word or follows a vowel, and a vowel otherwise: in "toy" the y is a
// consonant, in "syzygy" all three are vowels.
//
// Undefined if index is out of range.
bool is_consonant(std::string_view word, std::size_t index);

// Porter's measure m. Every word has the shape [C](VC){m}[V] where C is a run
// of consonants and V a run of vowels; m counts the (VC) repetitions.
//
//     m = 0    tr, ee, tree, y, by
//     m = 1    trouble, oats, trees, ivy
//     m = 2    troubles, private, oaten, orrery
//
// Later steps consult it before stripping a suffix: dropping "-ate" from
// "activate" is safe, dropping it from "gate" is not.
std::size_t measure(std::string_view word);

// Porter's *v* condition.
bool contains_vowel(std::string_view word);

// Porter's *d condition: the word ends in two identical consonants. "-ee" does
// not qualify, since those are vowels.
bool ends_with_double_consonant(std::string_view word);

// Porter's *o condition: the word ends consonant-vowel-consonant and that last
// consonant is not w, x or y.
//
//     hop, cat, fil    yes
//     snow, box, tray  no, excluded by the final letter
//     fail             no, the third-from-last letter is a vowel
//
// It marks short words that need an 'e' restored once a suffix comes off, so
// that "hop" and "hoping" agree on the spelling of their stem.
bool ends_cvc(std::string_view word);

// Step 1a, plurals. First match wins:
//
//     sses -> ss      caresses -> caress
//     ies  -> i       ponies   -> poni
//     ss   -> ss      caress   -> caress
//     s    -> ""      cats     -> cat
//
// The third rule exists only to block the fourth, which would otherwise strip
// "caress" down to "cares".
std::string step_1a_plurals(std::string_view word);

// Step 1b, past tense and progressive:
//
//     (m > 0) eed -> ee    agreed -> agree,  feed -> feed
//     (*v*)   ed  -> ""    plastered -> plaster,  bled -> bled
//     (*v*)   ing -> ""    motoring -> motor,  sing -> sing
//
// The eed rule measures what is left after removing the suffix: "agr" has
// m = 1 so it fires, "f" has m = 0 so it does not. It also ends the step
// either way, or "feed" would lose its "ed" to the next rule.
//
// If the ed or ing rule fired, a cleanup pass runs on the shortened word and
// likewise stops at its first match:
//
//     at, bl, iz -> add e                  conflat -> conflate
//     *d and last letter not l, s or z     hopp    -> hop
//     m == 1 and *o -> add e               fil     -> file
std::string step_1b_verb_endings(std::string_view word);

// Step 1c: (*v*) y -> i. "happy" becomes "happi" so that it meets the "happi"
// that step 3 leaves behind from "happiness". "sky" is left alone.
std::string step_1c_y_to_i(std::string_view word);

// Step 2, twenty rules under (m > 0) measured on the stem:
//
//     ational -> ate     relational  -> relate
//     tional  -> tion    conditional -> condition
//     izer    -> ize     digitizer   -> digitize
//     biliti  -> ble     sensibiliti -> sensible
//
// First match wins, and the table is ordered longest-first so that "ational"
// gets first refusal over "tional" and "ization" over "ation".
std::string step_2_double_suffixes(std::string_view word);

// Step 3, also under (m > 0). Some rules shorten, some delete:
//
//     icate -> ic        triplicate -> triplic
//     ative -> ""        formative  -> form
//     ful   -> ""        hopeful    -> hope
//     ness  -> ""        goodness   -> good
std::string step_3_derived_suffixes(std::string_view word);

// Step 4, the residual suffixes, deleted under the stricter (m > 1):
//
//     al ance ence er ic able ible ant ement ment ent ou ism ate iti ous ive ize
//
//     revival -> reviv        rival -> rival
//
// m > 1 rather than m > 0 because these endings are short and common; at the
// looser threshold "-al" would come off almost everything that ends in it.
//
// "ion" carries an extra condition and is handled separately: it goes only
// when the stem ends in s or t, so "adoption" stems to "adopt" but "lion"
// stays whole.
std::string step_4_residual_suffixes(std::string_view word);

// Step 5a, the final e:
//
//     (m > 1)            e -> ""     probate -> probat
//     (m = 1 and not *o) e -> ""     cease   -> ceas,  rate -> rate
//
// The second rule mirrors step 1b's cleanup, which adds an e at m = 1 when the
// stem does end cvc. Both spellings of a word therefore agree on where the e
// belongs.
std::string step_5a_final_e(std::string_view word);

// Step 5b: at (m > 1) a doubled l collapses. "controll", which is what step 4
// leaves behind from "controller", becomes "control"; "roll" has m = 1 and
// keeps both letters.
std::string step_5b_double_l(std::string_view word);

// Runs steps 1a, 1b, 1c, 2, 3, 4, 5a and 5b in order. Words of two letters or
// fewer come back untouched.
//
// The result is often not a word: "relational" stems to "relat". That is fine,
// since a stem is an index key rather than something a user sees. What matters
// is that related spellings agree on it, and that "connect", "connected",
// "connecting", "connection" and "connections" all reach "connect".
std::string stem(std::string_view word);

}
