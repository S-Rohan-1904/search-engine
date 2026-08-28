#include "index_io.hpp"

#include "index_map.hpp"

#include <cstdint>
#include <fstream>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace {

constexpr unsigned char kMagic[4] = {'S', 'I', 'D', 'X'};
constexpr std::uint32_t kVersion = 2;

class Writer {
public:
    explicit Writer(IndexEncoding encoding) : encoding_(encoding) {}

    void raw_byte(unsigned char value) { bytes_.push_back(value); }

    void fixed32(std::uint32_t value) {
        for (int shift = 0; shift < 32; shift += 8) {
            bytes_.push_back(static_cast<unsigned char>((value >> shift) & 0xFF));
        }
    }

    void number(std::uint64_t value) {
        if (encoding_ == IndexEncoding::VarByte) {
            while (value >= 128) {
                bytes_.push_back(static_cast<unsigned char>(value & 0x7F));
                value >>= 7;
            }
            bytes_.push_back(static_cast<unsigned char>(value | 0x80));
            return;
        }

        for (int shift = 0; shift < 64; shift += 8) {
            bytes_.push_back(static_cast<unsigned char>((value >> shift) & 0xFF));
        }
    }

    void text(std::string_view value) {
        number(value.size());
        bytes_.insert(bytes_.end(), value.begin(), value.end());
    }

    std::size_t size() const { return bytes_.size(); }

    std::vector<unsigned char> take() { return std::move(bytes_); }

private:
    IndexEncoding encoding_;
    std::vector<unsigned char> bytes_;
};

class Reader {
public:
    Reader(const std::vector<unsigned char>& bytes, IndexEncoding encoding)
        : bytes_(bytes), encoding_(encoding) {}

    void set_encoding(IndexEncoding encoding) { encoding_ = encoding; }

    bool raw_byte(unsigned char& out) {
        if (position_ >= bytes_.size()) {
            return false;
        }
        out = bytes_[position_++];
        return true;
    }

    bool fixed32(std::uint32_t& out) {
        if (bytes_.size() - position_ < 4) {
            return false;
        }
        out = 0;
        for (int shift = 0; shift < 32; shift += 8) {
            out |= static_cast<std::uint32_t>(bytes_[position_++]) << shift;
        }
        return true;
    }

    bool number(std::uint64_t& out) {
        if (encoding_ == IndexEncoding::VarByte) {
            out = 0;
            unsigned int shift = 0;
            while (position_ < bytes_.size()) {
                const unsigned char byte = bytes_[position_++];
                if (shift >= 64) {
                    return false;
                }
                out |= static_cast<std::uint64_t>(byte & 0x7F) << shift;
                if ((byte & 0x80) != 0) {
                    return true;
                }
                shift += 7;
            }
            return false;
        }

        if (bytes_.size() - position_ < 8) {
            return false;
        }
        out = 0;
        for (int shift = 0; shift < 64; shift += 8) {
            out |= static_cast<std::uint64_t>(bytes_[position_++]) << shift;
        }
        return true;
    }

    bool text(std::string& out) {
        std::uint64_t length = 0;
        if (!number(length) || bytes_.size() - position_ < length) {
            return false;
        }
        out.assign(bytes_.begin() + static_cast<std::ptrdiff_t>(position_),
                   bytes_.begin() + static_cast<std::ptrdiff_t>(position_ + length));
        position_ += length;
        return true;
    }

    bool at_end() const { return position_ == bytes_.size(); }

private:
    const std::vector<unsigned char>& bytes_;
    IndexEncoding encoding_;
    std::size_t position_ = 0;
};

bool uses_deltas(IndexEncoding encoding) {
    return encoding != IndexEncoding::Plain;
}

}

namespace {

}

std::vector<unsigned char> encode_index(const InvertedIndex& index, IndexEncoding encoding) {
    return encode_index_v3(index, encoding, nullptr);
}

IndexSizeReport measure_index(const InvertedIndex& index, IndexEncoding encoding) {
    IndexSizeReport report{};
    encode_index_v3(index, encoding, &report);
    return report;
}

std::optional<InvertedIndex> decode_index(const std::vector<unsigned char>& bytes,
                                          std::string& error) {
    error.clear();
    Reader reader(bytes, IndexEncoding::Plain);

    for (const unsigned char expected : kMagic) {
        unsigned char actual = 0;
        if (!reader.raw_byte(actual) || actual != expected) {
            error = "not an index file";
            return std::nullopt;
        }
    }

    std::uint32_t version = 0;
    if (!reader.fixed32(version) || version != kVersion) {
        error = "unsupported index version";
        return std::nullopt;
    }

    unsigned char encoding_byte = 0;
    if (!reader.raw_byte(encoding_byte) || encoding_byte > 2) {
        error = "unknown index encoding";
        return std::nullopt;
    }
    const IndexEncoding encoding = static_cast<IndexEncoding>(encoding_byte);
    reader.set_encoding(encoding);

    const auto truncated = [&error]() {
        error = "truncated index file";
        return std::nullopt;
    };

    std::uint64_t document_count = 0;
    if (!reader.number(document_count)) {
        return truncated();
    }

    std::vector<std::string> ids;
    std::vector<std::size_t> lengths;
    std::vector<std::uint64_t> fingerprints;
    ids.reserve(document_count);
    lengths.reserve(document_count);
    fingerprints.reserve(document_count);
    for (std::uint64_t i = 0; i < document_count; i++) {
        std::string id;
        std::uint64_t length = 0;
        std::uint64_t fingerprint = 0;
        if (!reader.text(id) || !reader.number(length) || !reader.number(fingerprint)) {
            return truncated();
        }
        ids.push_back(std::move(id));
        lengths.push_back(static_cast<std::size_t>(length));
        fingerprints.push_back(fingerprint);
    }

    std::uint64_t term_count = 0;
    if (!reader.number(term_count)) {
        return truncated();
    }

    std::unordered_map<std::string, std::vector<Posting>> postings_by_term;
    postings_by_term.reserve(term_count);
    for (std::uint64_t i = 0; i < term_count; i++) {
        std::string term;
        std::uint64_t posting_count = 0;
        if (!reader.text(term) || !reader.number(posting_count)) {
            return truncated();
        }

        std::vector<Posting> postings;
        postings.reserve(posting_count);

        std::size_t previous_doc_id = 0;
        for (std::uint64_t j = 0; j < posting_count; j++) {
            std::uint64_t doc_id = 0;
            std::uint64_t frequency = 0;
            std::uint64_t position_count = 0;
            if (!reader.number(doc_id) || !reader.number(frequency) ||
                !reader.number(position_count)) {
                return truncated();
            }

            const std::size_t absolute_doc_id =
                uses_deltas(encoding) ? previous_doc_id + static_cast<std::size_t>(doc_id)
                                      : static_cast<std::size_t>(doc_id);

            if (absolute_doc_id >= ids.size()) {
                error = "index refers to a document that is not in it";
                return std::nullopt;
            }
            if (j > 0 && absolute_doc_id <= postings.back().doc_id) {
                error = "postings list is not ascending";
                return std::nullopt;
            }
            previous_doc_id = absolute_doc_id;

            std::vector<std::size_t> positions;
            positions.reserve(position_count);
            std::size_t previous_position = 0;
            for (std::uint64_t p = 0; p < position_count; p++) {
                std::uint64_t position = 0;
                if (!reader.number(position)) {
                    return truncated();
                }
                const std::size_t absolute =
                    uses_deltas(encoding) ? previous_position + static_cast<std::size_t>(position)
                                          : static_cast<std::size_t>(position);
                if (p > 0 && absolute <= positions.back()) {
                    error = "positions are not ascending";
                    return std::nullopt;
                }
                previous_position = absolute;
                positions.push_back(absolute);
            }

            postings.push_back(
                Posting{absolute_doc_id, static_cast<std::size_t>(frequency), std::move(positions)});
        }

        postings_by_term.emplace(std::move(term), std::move(postings));
    }

    if (!reader.at_end()) {
        error = "trailing bytes after the index";
        return std::nullopt;
    }

    return InvertedIndex::from_parts(std::move(ids), std::move(lengths),
                                     std::move(fingerprints), std::move(postings_by_term));
}

bool write_index_file(const InvertedIndex& index, const std::filesystem::path& path,
                      IndexEncoding encoding, std::string& error) {
    error.clear();
    const std::vector<unsigned char> bytes = encode_index(index, encoding);

    std::ofstream out(path, std::ios::binary);
    if (!out) {
        error = "cannot write " + path.string();
        return false;
    }

    out.write(reinterpret_cast<const char*>(bytes.data()),
              static_cast<std::streamsize>(bytes.size()));
    if (!out) {
        error = "cannot write " + path.string();
        return false;
    }
    return true;
}

std::optional<InvertedIndex> read_index_file(const std::filesystem::path& path,
                                             std::string& error) {
    error.clear();
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "cannot read " + path.string();
        return std::nullopt;
    }

    // Version 3 is mapped rather than read, which is the whole point of it.
    // An older file has no tables to map, so it still goes through the decoder
    // that built the index in memory.
    std::string mapped_error;
    std::optional<MappedIndex> mapped = MappedIndex::open(path, mapped_error);
    if (mapped.has_value()) {
        return InvertedIndex::from_mapped(std::move(*mapped));
    }

    const std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)),
                                           std::istreambuf_iterator<char>());
    std::optional<InvertedIndex> decoded = decode_index(bytes, error);
    if (!decoded.has_value() && error.empty()) {
        error = mapped_error;
    }
    return decoded;
}

std::optional<IndexEncoding> parse_encoding(std::string_view name) {
    if (name == "plain") {
        return IndexEncoding::Plain;
    }
    if (name == "delta") {
        return IndexEncoding::Delta;
    }
    if (name == "varbyte") {
        return IndexEncoding::VarByte;
    }
    return std::nullopt;
}
