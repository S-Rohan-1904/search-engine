#include "index_map.hpp"

#include <algorithm>
#include <cstring>

namespace {

constexpr unsigned char kMagic[4] = {'S', 'I', 'D', 'X'};
constexpr std::uint32_t kVersion = 3;

// magic, version, encoding byte, then seven u64 header fields.
constexpr std::size_t kHeaderBytes = 4 + 4 + 1 + 7 * 8;

bool uses_deltas(IndexEncoding encoding) {
    return encoding != IndexEncoding::Plain;
}

void put_u64(std::vector<unsigned char>& out, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) {
        out.push_back(static_cast<unsigned char>((value >> shift) & 0xFF));
    }
}

void patch_u64(std::vector<unsigned char>& out, std::size_t at, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) {
        out[at++] = static_cast<unsigned char>((value >> shift) & 0xFF);
    }
}

std::uint64_t read_u64(const unsigned char* data, std::size_t at) {
    std::uint64_t value = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        value |= static_cast<std::uint64_t>(data[at++]) << shift;
    }
    return value;
}

std::uint32_t read_u32(const unsigned char* data, std::size_t at) {
    std::uint32_t value = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        value |= static_cast<std::uint32_t>(data[at++]) << shift;
    }
    return value;
}

void write_number(std::vector<unsigned char>& out, std::uint64_t value, IndexEncoding encoding) {
    if (encoding == IndexEncoding::VarByte) {
        while (value >= 128) {
            out.push_back(static_cast<unsigned char>(value & 0x7F));
            value >>= 7;
        }
        out.push_back(static_cast<unsigned char>(value | 0x80));
        return;
    }

    for (int shift = 0; shift < 64; shift += 8) {
        out.push_back(static_cast<unsigned char>((value >> shift) & 0xFF));
    }
}

// Reads one number and advances the cursor. The mapped file was validated when
// it was opened, so a cursor that runs past the end is a corrupt file rather
// than an expected case; it stops rather than reading on.
std::uint64_t read_number(const unsigned char* data, std::size_t size, std::size_t& at,
                         IndexEncoding encoding) {
    if (encoding == IndexEncoding::VarByte) {
        std::uint64_t value = 0;
        int shift = 0;
        while (at < size) {
            const unsigned char byte = data[at++];
            value |= static_cast<std::uint64_t>(byte & 0x7F) << shift;
            if ((byte & 0x80) != 0) {
                break;
            }
            shift += 7;
        }
        return value;
    }

    std::uint64_t value = 0;
    if (size - at >= 8) {
        value = read_u64(data, at);
        at += 8;
    }
    return value;
}

}

std::vector<unsigned char> encode_index_v3(const InvertedIndex& index, IndexEncoding encoding,
                                           IndexSizeReport* report) {
    std::vector<unsigned char> out;

    for (const unsigned char byte : kMagic) {
        out.push_back(byte);
    }
    for (int shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<unsigned char>((kVersion >> shift) & 0xFF));
    }
    out.push_back(static_cast<unsigned char>(encoding));

    const std::size_t counts_at = out.size();
    put_u64(out, 0);  // document_count
    put_u64(out, 0);  // term_count
    put_u64(out, 0);  // posting_count
    const std::size_t tables_at = out.size();
    put_u64(out, 0);  // lengths
    put_u64(out, 0);  // fingerprints
    put_u64(out, 0);  // id offsets
    put_u64(out, 0);  // term offsets

    IndexSizeReport sizes{};
    sizes.header = out.size();

    const std::vector<std::string>& ids = index.document_ids();
    const std::size_t documents = ids.size();
    const std::size_t lengths_at = out.size();
    for (std::size_t doc_id = 0; doc_id < documents; doc_id++) {
        put_u64(out, index.document_length(doc_id));
    }
    const std::size_t fingerprints_at = out.size();
    for (std::size_t doc_id = 0; doc_id < documents; doc_id++) {
        put_u64(out, index.document_fingerprint(doc_id));
    }

    // Ids as one blob plus a table of where each starts, so the last entry
    // bounds the one before it and reading id N never scans ids 0..N-1.
    const std::size_t id_offsets_at = out.size();
    for (std::size_t i = 0; i <= documents; i++) {
        put_u64(out, 0);
    }
    for (std::size_t doc_id = 0; doc_id < documents; doc_id++) {
        patch_u64(out, id_offsets_at + doc_id * 8, out.size());
        out.insert(out.end(), ids[doc_id].begin(), ids[doc_id].end());
    }
    patch_u64(out, id_offsets_at + documents * 8, out.size());
    sizes.documents = out.size() - sizes.header;

    const std::vector<std::string> terms = index.terms();
    const std::size_t term_offsets_at = out.size();
    for (std::size_t i = 0; i < terms.size(); i++) {
        put_u64(out, 0);
    }

    std::size_t mark = out.size();
    for (std::size_t slot = 0; slot < terms.size(); slot++) {
        patch_u64(out, term_offsets_at + slot * 8, out.size());

        const std::string& term = terms[slot];
        const std::vector<Posting>& postings = index.postings(term);

        write_number(out, term.size(), encoding);
        out.insert(out.end(), term.begin(), term.end());
        write_number(out, postings.size(), encoding);
        sizes.dictionary += out.size() - mark;
        mark = out.size();

        std::size_t previous_doc_id = 0;
        for (const Posting& posting : postings) {
            write_number(out, uses_deltas(encoding) ? posting.doc_id - previous_doc_id
                                                    : posting.doc_id,
                         encoding);
            previous_doc_id = posting.doc_id;

            write_number(out, posting.frequency, encoding);
            write_number(out, posting.positions.size(), encoding);
            sizes.postings += out.size() - mark;
            mark = out.size();

            std::size_t previous_position = 0;
            for (const std::size_t position : posting.positions) {
                write_number(out, uses_deltas(encoding) ? position - previous_position : position,
                             encoding);
                previous_position = position;
            }
            sizes.positions += out.size() - mark;
            mark = out.size();
        }
    }

    patch_u64(out, counts_at, documents);
    patch_u64(out, counts_at + 8, terms.size());
    patch_u64(out, counts_at + 16, index.posting_count());
    patch_u64(out, tables_at, lengths_at);
    patch_u64(out, tables_at + 8, fingerprints_at);
    patch_u64(out, tables_at + 16, id_offsets_at);
    patch_u64(out, tables_at + 24, term_offsets_at);

    sizes.total = out.size();
    if (report != nullptr) {
        *report = sizes;
    }
    return out;
}

std::optional<MappedIndex> MappedIndex::open(const std::filesystem::path& path,
                                             std::string& error) {
    error.clear();

    std::optional<MappedFile> mapped = MappedFile::open(path);
    if (!mapped.has_value()) {
        error = "cannot read " + path.string();
        return std::nullopt;
    }

    MappedIndex index;
    index.data_ = std::move(*mapped);

    const unsigned char* data = index.data_.data();
    const std::size_t size = index.data_.size();
    if (size < kHeaderBytes || !std::equal(std::begin(kMagic), std::end(kMagic), data)) {
        error = "not an index file";
        return std::nullopt;
    }
    if (read_u32(data, 4) != kVersion) {
        error = "unsupported index version";
        return std::nullopt;
    }

    const unsigned char encoding_byte = data[8];
    if (encoding_byte > 2) {
        error = "unknown index encoding";
        return std::nullopt;
    }
    index.encoding_ = static_cast<IndexEncoding>(encoding_byte);

    index.document_count_ = static_cast<std::size_t>(read_u64(data, 9));
    index.term_count_ = static_cast<std::size_t>(read_u64(data, 17));
    index.posting_count_ = static_cast<std::size_t>(read_u64(data, 25));
    index.lengths_at_ = static_cast<std::size_t>(read_u64(data, 33));
    index.fingerprints_at_ = static_cast<std::size_t>(read_u64(data, 41));
    index.id_offsets_at_ = static_cast<std::size_t>(read_u64(data, 49));
    index.term_offsets_at_ = static_cast<std::size_t>(read_u64(data, 57));

    // Every table is fixed width, so one bounds check each covers all of it.
    const std::size_t documents = index.document_count_;
    const std::size_t terms = index.term_count_;
    if (index.lengths_at_ > size || size - index.lengths_at_ < documents * 8 ||
        index.fingerprints_at_ > size || size - index.fingerprints_at_ < documents * 8 ||
        index.id_offsets_at_ > size || size - index.id_offsets_at_ < (documents + 1) * 8 ||
        index.term_offsets_at_ > size || size - index.term_offsets_at_ < terms * 8) {
        error = "truncated index file";
        return std::nullopt;
    }

    for (std::size_t doc_id = 0; doc_id < documents; doc_id++) {
        const std::uint64_t start = read_u64(data, index.id_offsets_at_ + doc_id * 8);
        const std::uint64_t end = read_u64(data, index.id_offsets_at_ + (doc_id + 1) * 8);
        if (start > end || end > size) {
            error = "index entry points outside the file";
            return std::nullopt;
        }
        index.total_document_length_ += read_u64(data, index.lengths_at_ + doc_id * 8);
    }

    for (std::size_t slot = 0; slot < terms; slot++) {
        if (read_u64(data, index.term_offsets_at_ + slot * 8) >= size) {
            error = "index entry points outside the file";
            return std::nullopt;
        }
    }

    return index;
}

std::size_t MappedIndex::document_length(std::size_t doc_id) const {
    if (doc_id >= document_count_) {
        return 0;
    }
    return static_cast<std::size_t>(read_u64(data_.data(), lengths_at_ + doc_id * 8));
}

std::uint64_t MappedIndex::document_fingerprint(std::size_t doc_id) const {
    if (doc_id >= document_count_) {
        return 0;
    }
    return read_u64(data_.data(), fingerprints_at_ + doc_id * 8);
}

std::string_view MappedIndex::document_id(std::size_t doc_id) const {
    if (doc_id >= document_count_) {
        return {};
    }
    const unsigned char* data = data_.data();
    const std::size_t start = static_cast<std::size_t>(read_u64(data, id_offsets_at_ + doc_id * 8));
    const std::size_t end =
        static_cast<std::size_t>(read_u64(data, id_offsets_at_ + (doc_id + 1) * 8));
    return std::string_view(reinterpret_cast<const char*>(data) + start, end - start);
}

std::string_view MappedIndex::term_at(std::size_t slot) const {
    const unsigned char* data = data_.data();
    std::size_t at = static_cast<std::size_t>(read_u64(data, term_offsets_at_ + slot * 8));
    const std::size_t length =
        static_cast<std::size_t>(read_number(data, data_.size(), at, encoding_));
    if (at + length > data_.size()) {
        return {};
    }
    return std::string_view(reinterpret_cast<const char*>(data) + at, length);
}

std::vector<Posting> MappedIndex::postings(std::string_view term) const {
    // The term table is sorted, so this is a binary search over mapped bytes:
    // nothing is decoded except the few term names the search lands on.
    std::size_t low = 0;
    std::size_t high = term_count_;
    while (low < high) {
        const std::size_t middle = low + (high - low) / 2;
        const std::string_view candidate = term_at(middle);
        if (candidate < term) {
            low = middle + 1;
        } else if (candidate > term) {
            high = middle;
        } else {
            return postings_at(middle);
        }
    }

    return {};
}

std::vector<Posting> MappedIndex::postings_at(std::size_t slot) const {
    if (slot >= term_count_) {
        return {};
    }

    const unsigned char* data = data_.data();
    const std::size_t size = data_.size();
    std::size_t at = static_cast<std::size_t>(read_u64(data, term_offsets_at_ + slot * 8));
    const std::size_t length =
        static_cast<std::size_t>(read_number(data, size, at, encoding_));
    at += length;

    const std::size_t count =
        static_cast<std::size_t>(read_number(data, size, at, encoding_));

    std::vector<Posting> out;
    out.reserve(count);

    std::size_t doc_id = 0;
    for (std::size_t i = 0; i < count; i++) {
        const std::uint64_t stored = read_number(data, size, at, encoding_);
        doc_id = uses_deltas(encoding_) ? doc_id + static_cast<std::size_t>(stored)
                                        : static_cast<std::size_t>(stored);

        Posting posting;
        posting.doc_id = doc_id;
        posting.frequency = static_cast<std::size_t>(read_number(data, size, at, encoding_));

        const std::size_t positions =
            static_cast<std::size_t>(read_number(data, size, at, encoding_));
        posting.positions.reserve(positions);

        std::size_t position = 0;
        for (std::size_t p = 0; p < positions; p++) {
            const std::uint64_t raw = read_number(data, size, at, encoding_);
            position = uses_deltas(encoding_) ? position + static_cast<std::size_t>(raw)
                                              : static_cast<std::size_t>(raw);
            posting.positions.push_back(position);
        }

        out.push_back(std::move(posting));
    }
    return out;
}

std::vector<Token> MappedIndex::document_terms(std::size_t doc_id) const {
    std::vector<Token> out;

    for (std::size_t slot = 0; slot < term_count_; slot++) {
        const std::string_view term = term_at(slot);
        for (const Posting& posting : postings(term)) {
            if (posting.doc_id != doc_id) {
                continue;
            }
            for (const std::size_t position : posting.positions) {
                out.push_back(Token{std::string(term), position, 0});
            }
            break;
        }
    }

    std::sort(out.begin(), out.end(),
              [](const Token& a, const Token& b) { return a.position < b.position; });
    return out;
}

std::vector<std::string> MappedIndex::terms() const {
    std::vector<std::string> out;
    out.reserve(term_count_);
    for (std::size_t slot = 0; slot < term_count_; slot++) {
        out.emplace_back(term_at(slot));
    }
    return out;
}
