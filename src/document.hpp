#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

struct Document {
    std::string id;     // filename without the .txt extension
    std::string title;
    std::string body;
};

// Parses the on-disk document format: a "title:" header line, then the body.
//
//     title: Cats and Mats
//
//     The cat sat on the mat. ...
//
// Line one must have a colon and a key of exactly "title" once trimmed,
// otherwise the document is malformed and this returns nullopt. The title is
// whatever follows the colon, trimmed. The body is the remaining lines with
// blank lines stripped from each end but kept in the middle, and no trailing
// newline. Empty titles and empty bodies are both legal, and the blank line
// after the header is conventional rather than required.
std::optional<Document> parse_document(std::string id, std::string_view raw);

// Reads and parses corpus_dir/<id>.txt. Returns nullopt if it cannot be read
// or is malformed.
std::optional<Document> read_document(const std::filesystem::path& corpus_dir,
                                      const std::string& id);
