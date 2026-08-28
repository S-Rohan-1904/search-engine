#include "corpus_file.hpp"



#include <algorithm>
#include <fstream>
#include <iterator>
#include <utility>

#include "corpus.hpp"

namespace {

constexpr unsigned char kMagic[4] = {'S', 'C', 'P', 'S'};
constexpr std::uint32_t kVersion = 1;

void put_u32(std::vector<unsigned char>& out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<unsigned char>((value >> shift) & 0xFF));
    }
}

void put_u64(std::vector<unsigned char>& out, std::uint64_t value) {
    for (int shift = 0; shift < 64; shift += 8) {
        out.push_back(static_cast<unsigned char>((value >> shift) & 0xFF));
    }
}

std::uint32_t get_u32(const unsigned char* data, std::size_t at) {
    std::uint32_t value = 0;
    for (int shift = 0; shift < 32; shift += 8) {
        value |= static_cast<std::uint32_t>(data[at++]) << shift;
    }
    return value;
}

std::uint64_t get_u64(const unsigned char* data, std::size_t at) {
    std::uint64_t value = 0;
    for (int shift = 0; shift < 64; shift += 8) {
        value |= static_cast<std::uint64_t>(data[at++]) << shift;
    }
    return value;
}

std::uint64_t hash_bytes(const unsigned char* data, std::size_t length) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (std::size_t i = 0; i < length; i++) {
        hash ^= data[i];
        hash *= 1099511628211ULL;
    }
    return hash == 0 ? 1 : hash;
}

constexpr std::size_t kHeaderBytes = 4 + 4 + 8 + 8;

}

std::optional<CorpusFile> CorpusFile::open(const std::filesystem::path& path,
                                           std::string& error) {
    error.clear();

    std::optional<MappedFile> mapped = MappedFile::open(path);
    if (!mapped.has_value()) {
        error = "cannot read " + path.string();
        return std::nullopt;
    }

    CorpusFile corpus;
    corpus.data_ = std::move(*mapped);

    if (corpus.data_.size() < kHeaderBytes ||
        !std::equal(std::begin(kMagic), std::end(kMagic), corpus.data_.data())) {
        error = "not a corpus file";
        return std::nullopt;
    }

    if (get_u32(corpus.data_.data(), 4) != kVersion) {
        error = "unsupported corpus version";
        return std::nullopt;
    }

    const std::uint64_t count = get_u64(corpus.data_.data(), 8);
    const std::uint64_t table_at = get_u64(corpus.data_.data(), 16);
    if (table_at > corpus.data_.size()) {
        error = "truncated corpus file";
        return std::nullopt;
    }

    corpus.ids_.reserve(count);
    corpus.entries_.reserve(count);

    std::size_t cursor = static_cast<std::size_t>(table_at);
    for (std::uint64_t i = 0; i < count; i++) {
        if (corpus.data_.size() - cursor < 4) {
            error = "truncated corpus file";
            return std::nullopt;
        }
        const std::uint32_t id_length = get_u32(corpus.data_.data(), cursor);
        cursor += 4;

        if (corpus.data_.size() - cursor < static_cast<std::size_t>(id_length) + 16) {
            error = "truncated corpus file";
            return std::nullopt;
        }

        corpus.ids_.emplace_back(reinterpret_cast<const char*>(corpus.data_.data()) + cursor,
                                 id_length);
        cursor += id_length;

        const std::uint64_t offset = get_u64(corpus.data_.data(), cursor);
        cursor += 8;
        const std::uint64_t length = get_u64(corpus.data_.data(), cursor);
        cursor += 8;

        if (offset > table_at || length > table_at - offset) {
            error = "corpus entry points outside the file";
            return std::nullopt;
        }

        corpus.entries_.push_back(
            Entry{static_cast<std::size_t>(offset), static_cast<std::size_t>(length)});
    }

    if (cursor != corpus.data_.size()) {
        error = "trailing bytes after the corpus";
        return std::nullopt;
    }

    corpus.order_.resize(corpus.ids_.size());
    for (std::size_t i = 0; i < corpus.order_.size(); i++) {
        corpus.order_[i] = i;
    }
    std::sort(corpus.order_.begin(), corpus.order_.end(),
              [&corpus](std::size_t a, std::size_t b) {
                  return corpus.ids_[a] < corpus.ids_[b];
              });

    return corpus;
}

std::size_t CorpusFile::position_of(const std::string& id) const {
    const auto it = std::lower_bound(order_.begin(), order_.end(), id,
                                     [this](std::size_t position, const std::string& target) {
                                         return ids_[position] < target;
                                     });
    if (it == order_.end() || ids_[*it] != id) {
        return ids_.size();
    }
    return *it;
}

std::optional<Document> CorpusFile::read(const std::string& id) const {
    const std::size_t position = position_of(id);
    if (position == ids_.size()) {
        return std::nullopt;
    }

    const Entry& entry = entries_[position];
    const std::string_view raw(reinterpret_cast<const char*>(data_.data()) + entry.offset,
                               entry.length);
    return parse_document(id, raw);
}

std::uint64_t CorpusFile::fingerprint(const std::string& id) const {
    const std::size_t position = position_of(id);
    if (position == ids_.size()) {
        return 0;
    }

    const Entry& entry = entries_[position];
    return hash_bytes(data_.data() + entry.offset, entry.length);
}

std::optional<CorpusWriter> CorpusWriter::create(const std::filesystem::path& output,
                                                 std::string& error) {
    error.clear();

    CorpusWriter writer;
    writer.out_.open(output, std::ios::binary | std::ios::trunc);
    if (!writer.out_) {
        error = "cannot write " + output.string();
        return std::nullopt;
    }

    const std::vector<unsigned char> placeholder(kHeaderBytes, 0);
    writer.out_.write(reinterpret_cast<const char*>(placeholder.data()),
                      static_cast<std::streamsize>(placeholder.size()));
    writer.cursor_ = kHeaderBytes;
    return writer;
}

bool CorpusWriter::add(const std::string& id, std::string_view raw) {
    if (!out_) {
        return false;
    }

    out_.write(raw.data(), static_cast<std::streamsize>(raw.size()));
    if (!out_) {
        return false;
    }

    ids_.push_back(id);
    placed_.push_back(Placed{cursor_, raw.size()});
    cursor_ += raw.size();
    return true;
}

bool CorpusWriter::finish(std::string& error) {
    error.clear();
    if (!out_) {
        error = "corpus writer is not open";
        return false;
    }

    const std::size_t table_at = cursor_;

    std::vector<unsigned char> table;
    for (std::size_t i = 0; i < ids_.size(); i++) {
        put_u32(table, static_cast<std::uint32_t>(ids_[i].size()));
        table.insert(table.end(), ids_[i].begin(), ids_[i].end());
        put_u64(table, placed_[i].offset);
        put_u64(table, placed_[i].length);
    }

    out_.write(reinterpret_cast<const char*>(table.data()),
               static_cast<std::streamsize>(table.size()));

    std::vector<unsigned char> header;
    header.insert(header.end(), std::begin(kMagic), std::end(kMagic));
    put_u32(header, kVersion);
    put_u64(header, ids_.size());
    put_u64(header, table_at);

    out_.seekp(0);
    out_.write(reinterpret_cast<const char*>(header.data()),
               static_cast<std::streamsize>(header.size()));
    out_.flush();

    if (!out_) {
        error = "cannot finish writing the corpus";
        return false;
    }
    return true;
}

bool write_corpus_file(const std::filesystem::path& corpus_dir,
                       const std::filesystem::path& output, std::string& error) {
    std::optional<CorpusWriter> writer = CorpusWriter::create(output, error);
    if (!writer.has_value()) {
        return false;
    }

    for (const std::string& id : list_document_ids(corpus_dir)) {
        std::ifstream in(corpus_dir / (id + ".txt"), std::ios::binary);
        if (!in) {
            continue;
        }

        const std::string raw((std::istreambuf_iterator<char>(in)),
                              std::istreambuf_iterator<char>());
        if (!writer->add(id, raw)) {
            error = "cannot write " + output.string();
            return false;
        }
    }

    return writer->finish(error);
}
