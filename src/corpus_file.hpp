#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "document.hpp"
#include "mapped_file.hpp"

// A whole corpus in one file.
//
// One .txt per document is the obvious layout and it stops working at scale.
// A quarter of a million documents averaging 200 bytes needs a quarter of a
// million inodes, costs a 4 KB block each on most filesystems, and makes every
// index build a quarter of a million open() calls.
//
// The container is a header, the documents back to back, then a table of
// (id, offset, length). Reading a document is an offset into a buffer already
// in memory rather than a syscall.
//
// The format mirrors the index format from stage 26 deliberately: magic bytes so
// a wrong file is an error rather than a wild allocation, a version field that
// is checked, and a fixed little-endian layout so the file does not depend on
// the machine that wrote it.
class CorpusFile {
public:
    // Reads and validates a .corpus file.
    //
    // Returns nullopt with `error` set on a bad magic number, an unknown
    // version, a truncated file, or a table entry pointing outside it.
    static std::optional<CorpusFile> open(const std::filesystem::path& path, std::string& error);

    const std::vector<std::string>& document_ids() const { return ids_; }

    // The document with this id, or nullopt if it is absent or malformed.
    std::optional<Document> read(const std::string& id) const;

    // A hash of the document's stored bytes, matching what fingerprint_document
    // computes for the same document on disk.
    std::uint64_t fingerprint(const std::string& id) const;

    std::uint64_t bytes() const { return static_cast<std::uint64_t>(data_.size()); }

    // A corpus file is mapped, not copied, so it is moved rather than copied.
    CorpusFile(CorpusFile&&) noexcept = default;
    CorpusFile& operator=(CorpusFile&&) noexcept = default;
    CorpusFile(const CorpusFile&) = delete;
    CorpusFile& operator=(const CorpusFile&) = delete;
    CorpusFile() = default;

private:
    struct Entry {
        std::size_t offset;
        std::size_t length;
    };

    // Positions into ids_, ordered by the id they name, so a lookup is a binary
    // search rather than a scan. Reading a document by id happens once per
    // document per index build, so a linear scan here is quadratic over the
    // corpus and does not show up until the corpus is large.
    std::size_t position_of(const std::string& id) const;

    MappedFile data_;
    std::vector<std::string> ids_;
    std::vector<Entry> entries_;
    std::vector<std::size_t> order_;
};

// Writes a .corpus file one document at a time.
//
// Streaming rather than building the whole thing in memory, because the largest
// corpus this is meant for does not fit: 6.9M documents of a few hundred bytes
// is well over a gigabyte before the table is counted.
//
// Documents go straight to the file as they arrive; only the offset table is
// held in memory, and the header is patched at the end once the table's
// position is known. Documents must be added in the order their ids sort, since
// that order fixes the ordinals an index will assign.
class CorpusWriter {
public:
    // Opens `output` and reserves space for the header.
    static std::optional<CorpusWriter> create(const std::filesystem::path& output,
                                              std::string& error);

    CorpusWriter() = default;
    CorpusWriter(CorpusWriter&&) = default;
    CorpusWriter& operator=(CorpusWriter&&) = default;

    bool add(const std::string& id, std::string_view raw);

    // Writes the table and patches the header. The file is incomplete until
    // this succeeds.
    bool finish(std::string& error);

    std::size_t count() const { return ids_.size(); }

private:
    struct Placed {
        std::size_t offset;
        std::size_t length;
    };

    std::ofstream out_;
    std::vector<std::string> ids_;
    std::vector<Placed> placed_;
    std::size_t cursor_ = 0;
};

// Packs every document in `corpus_dir` into one .corpus file.
//
// Documents are written in the sorted id order list_document_ids returns, so
// the container preserves the ordinals an index built from the directory would
// have assigned. That is what lets an index built from either be identical.
bool write_corpus_file(const std::filesystem::path& corpus_dir,
                       const std::filesystem::path& output, std::string& error);
