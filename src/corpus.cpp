#include "corpus.hpp"

#include <fstream>
#include <utility>

#include "corpus_file.hpp"
#include "document.hpp"

#include <algorithm>

std::vector<std::string> list_document_ids(const std::filesystem::path& corpus_dir) {
    std::vector<std::string> document_ids;

    std::error_code ec;
    for(const auto & entry : std::filesystem::directory_iterator(corpus_dir, ec)) {
        if(entry.is_regular_file() && entry.path().extension() == ".txt") {
            document_ids.push_back(entry.path().stem().string());
        }
    }

    std::sort(document_ids.begin(), document_ids.end());

    return document_ids;
}

std::uint64_t fingerprint_file(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return 0;
    }

    std::uint64_t hash = 1469598103934665603ULL;
    char buffer[4096];
    while (in.read(buffer, sizeof(buffer)) || in.gcount() > 0) {
        for (std::streamsize i = 0; i < in.gcount(); i++) {
            hash ^= static_cast<unsigned char>(buffer[i]);
            hash *= 1099511628211ULL;
        }
    }

    return hash == 0 ? 1 : hash;
}

namespace {

class DirectoryCorpus : public CorpusReader {
public:
    explicit DirectoryCorpus(std::filesystem::path root)
        : root_(std::move(root)), ids_(list_document_ids(root_)) {}

    const std::vector<std::string>& document_ids() const override { return ids_; }

    std::optional<Document> read(const std::string& id) const override {
        return read_document(root_, id);
    }

    std::uint64_t fingerprint(const std::string& id) const override {
        return fingerprint_file(root_ / (id + ".txt"));
    }

private:
    std::filesystem::path root_;
    std::vector<std::string> ids_;
};

class PackedCorpus : public CorpusReader {
public:
    explicit PackedCorpus(CorpusFile file) : file_(std::move(file)) {}

    const std::vector<std::string>& document_ids() const override {
        return file_.document_ids();
    }

    std::optional<Document> read(const std::string& id) const override {
        return file_.read(id);
    }

    std::uint64_t fingerprint(const std::string& id) const override {
        return file_.fingerprint(id);
    }

private:
    CorpusFile file_;
};

}

std::unique_ptr<CorpusReader> open_corpus(const std::filesystem::path& source,
                                          std::string& error) {
    error.clear();

    std::error_code ec;
    if (std::filesystem::is_directory(source, ec)) {
        return std::make_unique<DirectoryCorpus>(source);
    }

    if (std::filesystem::is_regular_file(source, ec)) {
        std::optional<CorpusFile> file = CorpusFile::open(source, error);
        if (!file.has_value()) {
            return nullptr;
        }
        return std::make_unique<PackedCorpus>(std::move(*file));
    }

    error = "not a directory: " + source.string();
    return nullptr;
}
