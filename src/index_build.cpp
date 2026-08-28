#include "index_build.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <queue>
#include <vector>

#include "analyzer.hpp"
#include "corpus.hpp"
#include "index.hpp"
#include "index_io.hpp"
#include "index_map.hpp"

namespace {

using Clock = std::chrono::steady_clock;

double elapsed_ms(Clock::time_point from, Clock::time_point to) {
    return std::chrono::duration<double, std::milli>(to - from).count();
}

// The output is written in the same version 3 layout the in-memory encoder
// produces, but streamed: the tables whose contents are only known at the end
// are written after the data they describe, and the header points at them.
class StreamWriter {
public:
    explicit StreamWriter(std::ofstream& out) : out_(out) {}

    void byte(unsigned char value) {
        out_.put(static_cast<char>(value));
        cursor_++;
    }

    void fixed32(std::uint32_t value) {
        for (int shift = 0; shift < 32; shift += 8) {
            byte(static_cast<unsigned char>((value >> shift) & 0xFF));
        }
    }

    void fixed64(std::uint64_t value) {
        for (int shift = 0; shift < 64; shift += 8) {
            byte(static_cast<unsigned char>((value >> shift) & 0xFF));
        }
    }

    void varbyte(std::uint64_t value) {
        while (value >= 128) {
            byte(static_cast<unsigned char>(value & 0x7F));
            value >>= 7;
        }
        byte(static_cast<unsigned char>(value | 0x80));
    }

    void text(std::string_view value) {
        out_.write(value.data(), static_cast<std::streamsize>(value.size()));
        cursor_ += value.size();
    }

    std::size_t cursor() const { return cursor_; }

private:
    std::ofstream& out_;
    std::size_t cursor_ = 0;
};

// One block file, positioned at a term, waiting its turn in the merge.
struct Cursor {
    MappedIndex index;
    std::size_t slot = 0;
    std::size_t base = 0;  // this block's first document ordinal in the output
};

constexpr unsigned char kMagic[4] = {'S', 'I', 'D', 'X'};
constexpr std::uint32_t kVersion = 3;

}

bool build_index_external(const std::filesystem::path& source,
                          const std::filesystem::path& output, std::size_t block_documents,
                          std::size_t threads, ExternalBuildReport& report, std::string& error) {
    report = ExternalBuildReport{};
    error.clear();
    (void)threads;

    if (block_documents == 0) {
        error = "block size must be at least one document";
        return false;
    }

    const std::unique_ptr<CorpusReader> corpus = open_corpus(source, error);
    if (!corpus) {
        return false;
    }

    const std::vector<std::string>& ids = corpus->document_ids();

    // Blocks go beside the output, so they land on the same filesystem and are
    // covered by whatever space check the caller already did on it.
    const std::filesystem::path block_prefix = output.string() + ".block";
    std::vector<std::filesystem::path> block_paths;

    const auto started = Clock::now();

    for (std::size_t first = 0; first < ids.size(); first += block_documents) {
        const std::size_t last = std::min(first + block_documents, ids.size());

        InvertedIndex block;
        for (std::size_t i = first; i < last; i++) {
            const std::optional<Document> doc = corpus->read(ids[i]);
            if (doc.has_value()) {
                block.add_document(ids[i], analyze_document(*doc), corpus->fingerprint(ids[i]));
            }
        }

        const std::filesystem::path path =
            block_prefix.string() + std::to_string(block_paths.size());
        if (!write_index_file(block, path, IndexEncoding::VarByte, error)) {
            return false;
        }
        block_paths.push_back(path);
    }

    const auto indexed = Clock::now();
    report.index_ms = elapsed_ms(started, indexed);
    report.blocks = block_paths.size();

    const auto clean_up = [&block_paths]() {
        for (const std::filesystem::path& path : block_paths) {
            std::error_code ignored;
            std::filesystem::remove(path, ignored);
        }
    };

    std::vector<Cursor> cursors;
    cursors.reserve(block_paths.size());
    std::size_t base = 0;
    for (const std::filesystem::path& path : block_paths) {
        std::optional<MappedIndex> mapped = MappedIndex::open(path, error);
        if (!mapped.has_value()) {
            clean_up();
            return false;
        }
        const std::size_t documents = mapped->document_count();
        cursors.push_back(Cursor{std::move(*mapped), 0, base});
        base += documents;
    }
    report.documents = base;

    std::ofstream out(output, std::ios::binary);
    if (!out) {
        error = "cannot write " + output.string();
        clean_up();
        return false;
    }

    StreamWriter writer(out);
    for (const unsigned char value : kMagic) {
        writer.byte(value);
    }
    writer.fixed32(kVersion);
    writer.byte(static_cast<unsigned char>(IndexEncoding::VarByte));

    const std::size_t header_at = writer.cursor();
    for (int i = 0; i < 7; i++) {
        writer.fixed64(0);
    }

    const std::size_t lengths_at = writer.cursor();
    for (const Cursor& cursor : cursors) {
        for (std::size_t doc_id = 0; doc_id < cursor.index.document_count(); doc_id++) {
            writer.fixed64(cursor.index.document_length(doc_id));
        }
    }

    const std::size_t fingerprints_at = writer.cursor();
    for (const Cursor& cursor : cursors) {
        for (std::size_t doc_id = 0; doc_id < cursor.index.document_count(); doc_id++) {
            writer.fixed64(cursor.index.document_fingerprint(doc_id));
        }
    }

    // The offset tables come before the data they describe, matching what the
    // in-memory encoder writes, so the two produce the same bytes. Their
    // contents are only known afterwards, so space is reserved here and filled
    // in at the end along with the header.
    const std::size_t id_offsets_at = writer.cursor();
    for (std::size_t i = 0; i <= report.documents; i++) {
        writer.fixed64(0);
    }

    std::vector<std::uint64_t> id_offsets;
    id_offsets.reserve(report.documents + 1);
    for (const Cursor& cursor : cursors) {
        for (std::size_t doc_id = 0; doc_id < cursor.index.document_count(); doc_id++) {
            id_offsets.push_back(writer.cursor());
            writer.text(cursor.index.document_id(doc_id));
        }
    }
    id_offsets.push_back(writer.cursor());

    // The merge. Every block's terms are sorted, so the next term to write is
    // the smallest across the cursors, and only the blocks holding it advance.
    // How many distinct terms the merge will write. The table has to be sized
    // exactly, because the entries follow it and an over-long table would push
    // them along and produce a different file from the in-memory encoder's.
    //
    // Counting is a pass over the term names alone -- no postings are decoded,
    // and the names are already in mapped memory -- so it costs a walk of the
    // dictionaries rather than a second look at the corpus.
    std::size_t term_count = 0;
    {
        std::vector<std::size_t> slots(cursors.size(), 0);
        while (true) {
            std::string_view smallest;
            bool found = false;
            for (std::size_t i = 0; i < cursors.size(); i++) {
                if (slots[i] >= cursors[i].index.term_count()) {
                    continue;
                }
                const std::string_view candidate = cursors[i].index.term_at(slots[i]);
                if (!found || candidate < smallest) {
                    smallest = candidate;
                    found = true;
                }
            }
            if (!found) {
                break;
            }
            for (std::size_t i = 0; i < cursors.size(); i++) {
                if (slots[i] < cursors[i].index.term_count() &&
                    cursors[i].index.term_at(slots[i]) == smallest) {
                    slots[i]++;
                }
            }
            term_count++;
        }
    }

    const std::size_t term_offsets_at = writer.cursor();
    for (std::size_t i = 0; i < term_count; i++) {
        writer.fixed64(0);
    }

    std::vector<std::uint64_t> term_offsets;

    while (true) {
        std::string_view smallest;
        bool found = false;
        for (const Cursor& cursor : cursors) {
            if (cursor.slot >= cursor.index.term_count()) {
                continue;
            }
            const std::string_view candidate = cursor.index.term_at(cursor.slot);
            if (!found || candidate < smallest) {
                smallest = candidate;
                found = true;
            }
        }
        if (!found) {
            break;
        }

        // Postings are gathered in block order, and blocks partition the
        // documents in ordinal order, so the merged list is already ascending.
        std::vector<Posting> merged;
        for (Cursor& cursor : cursors) {
            if (cursor.slot >= cursor.index.term_count() ||
                cursor.index.term_at(cursor.slot) != smallest) {
                continue;
            }
            for (Posting& posting : cursor.index.postings_at(cursor.slot)) {
                posting.doc_id += cursor.base;
                merged.push_back(std::move(posting));
            }
            cursor.slot++;
        }

        term_offsets.push_back(writer.cursor());
        writer.varbyte(smallest.size());
        writer.text(smallest);
        writer.varbyte(merged.size());

        std::size_t previous_doc_id = 0;
        for (const Posting& posting : merged) {
            writer.varbyte(posting.doc_id - previous_doc_id);
            previous_doc_id = posting.doc_id;

            writer.varbyte(posting.frequency);
            writer.varbyte(posting.positions.size());

            std::size_t previous_position = 0;
            for (const std::size_t position : posting.positions) {
                writer.varbyte(position - previous_position);
                previous_position = position;
            }
        }

        report.postings += merged.size();
    }

    report.terms = term_offsets.size();

    if (!out) {
        error = "cannot write " + output.string();
        clean_up();
        return false;
    }

    const auto patch = [&out](std::size_t at, const std::vector<std::uint64_t>& values) {
        out.seekp(static_cast<std::streamoff>(at));
        for (const std::uint64_t value : values) {
            for (int shift = 0; shift < 64; shift += 8) {
                out.put(static_cast<char>((value >> shift) & 0xFF));
            }
        }
    };

    patch(id_offsets_at, id_offsets);
    patch(term_offsets_at, term_offsets);

    // Everything the header points at is now placed, so go back and fill it in.
    out.seekp(static_cast<std::streamoff>(header_at));
    const std::uint64_t fields[7] = {report.documents,  report.terms,   report.postings,
                                     lengths_at,        fingerprints_at, id_offsets_at,
                                     term_offsets_at};
    for (const std::uint64_t value : fields) {
        for (int shift = 0; shift < 64; shift += 8) {
            out.put(static_cast<char>((value >> shift) & 0xFF));
        }
    }

    out.close();
    clean_up();

    if (!out) {
        error = "cannot write " + output.string();
        return false;
    }

    report.merge_ms = elapsed_ms(indexed, Clock::now());
    return true;
}
