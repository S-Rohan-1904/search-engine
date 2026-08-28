#pragma once

#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <string>

// What an import produced.
struct WikiImportReport {
    std::size_t lines_seen = 0;
    std::size_t documents_written = 0;
    std::size_t skipped_empty = 0;
};

// Which field of a CirrusSearch record becomes the document body.
enum class WikiField {
    Text,         // the whole article, already plain text
    OpeningText,  // the opening paragraph only
};

// Converts a Wikipedia CirrusSearch dump into a .corpus file.
//
// CirrusSearch dumps are what Wikipedia's own search runs on, and they carry
// article text that MediaWiki has already stripped of templates, tables and
// markup. That is the reason for using them over pages-articles: the wikitext
// stripper is the hard part of importing Wikipedia, and this dump has had it
// done properly upstream.
//
// The format is JSON Lines in pairs: an index-action line, then the document.
// Rather than assume the alternation, every line is searched for the fields it
// needs and skipped if they are absent.
//
// Scanned rather than parsed: only two string fields out of about twenty are
// wanted, and a JSON parser would cost a dependency to read fields a search can
// find. Two parts of that shortcut are not optional, because getting either
// wrong corrupts text silently rather than failing:
//
//   * A key match must check that the key really is one. Searching for
//     "text":" finds it inside "opening_text":" and "auxiliary_text":" as well,
//     so the byte before the opening quote has to be '{' or ','.
//   * String unescaping must be complete: the six short escapes, the escaped
//     quote, slash and backslash, and \uXXXX including surrogate pairs, which
//     are how JSON carries anything outside the basic multilingual plane.
//
// Reads from `in` in chunks, so a multi-gigabyte dump can be piped through
// without ever being held in memory or written to disk.
//
// `limit` of 0 imports everything.
bool import_cirrussearch(std::istream& in, const std::filesystem::path& output,
                         WikiField field, std::size_t limit, WikiImportReport& report,
                         std::string& error);
