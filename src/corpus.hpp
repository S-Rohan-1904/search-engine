#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "document.hpp"

// Lists the ids of every .txt file directly inside corpus_dir, sorted
// ascending. Subdirectories and other extensions are ignored. An id is the
// filename without the extension: corpus/doc_007.txt -> "doc_007".
//
// A directory that cannot be read yields an empty list rather than an
// exception. A caller that needs to tell "empty" from "missing" should check
// the path itself.
std::vector<std::string> list_document_ids(const std::filesystem::path& corpus_dir);

// A hash of a file's bytes, or 0 if it cannot be read.
//
// FNV-1a, the same function the crawler uses for duplicate detection, so a
// document has one fingerprint whether it is measured on disk or inside a
// container.
std::uint64_t fingerprint_file(const std::filesystem::path& path);

// Where documents come from.
//
// A corpus is either a directory of .txt files or one packed container, and
// nothing above this cares which. Indexing, updating and every command that
// reads a document go through this interface, so supporting a new layout means
// one implementation rather than a change at every call site.
//
// Implementations must be safe to read from several threads at once, since the
// parallel build shares one reader across its workers.
class CorpusReader {
public:
    virtual ~CorpusReader() = default;

    // Every document id, sorted ascending. The order fixes the ordinals an
    // index will assign, so it must not depend on the storage layout.
    virtual const std::vector<std::string>& document_ids() const = 0;

    virtual std::optional<Document> read(const std::string& id) const = 0;
    virtual std::uint64_t fingerprint(const std::string& id) const = 0;
};

// Opens a corpus directory or a .corpus file. Returns nullptr with `error` set
// if the path is neither.
std::unique_ptr<CorpusReader> open_corpus(const std::filesystem::path& source,
                                          std::string& error);
