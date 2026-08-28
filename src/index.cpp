#include "index.hpp"

#include <algorithm>
#include <fstream>
#include <future>
#include <iterator>
#include <optional>
#include <string>
#include <utility>

#include "analyzer.hpp"
#include "corpus.hpp"
#include "document.hpp"
#include "thread_pool.hpp"

void InvertedIndex::add_document(std::string doc_id, const std::vector<Token>& terms,
                                 std::uint64_t fingerprint) {
    const std::size_t ordinal = document_ids_.size();
    document_ids_.push_back(std::move(doc_id));
    document_lengths_.push_back(terms.size());
    document_fingerprints_.push_back(fingerprint);

    for (const Token& token : terms) {
        std::vector<Posting>& postings = postings_[token.text];
        if (postings.empty() || postings.back().doc_id != ordinal) {
            postings.push_back(Posting{ordinal, 1, {token.position}});
        } else {
            postings.back().frequency++;
            postings.back().positions.push_back(token.position);
        }
    }
}

std::size_t InvertedIndex::document_count() const {
    return document_ids_.size();
}

std::size_t InvertedIndex::term_count() const {
    return postings_.size();
}

std::size_t InvertedIndex::posting_count() const {
    std::size_t total = 0;
    for (const auto& [term, postings] : postings_) {
        total += postings.size();
    }
    return total;
}

std::size_t InvertedIndex::document_frequency(std::string_view term) const {
    return postings(term).size();
}

const std::vector<Posting>& InvertedIndex::postings(std::string_view term) const {
    static const std::vector<Posting> kEmpty;

    const auto it = postings_.find(std::string(term));
    return it == postings_.end() ? kEmpty : it->second;
}

std::size_t InvertedIndex::document_length(std::size_t doc_id) const {
    return doc_id < document_lengths_.size() ? document_lengths_[doc_id] : 0;
}

double InvertedIndex::average_document_length() const {
    if (document_lengths_.empty()) {
        return 0.0;
    }

    std::size_t total = 0;
    for (const std::size_t length : document_lengths_) {
        total += length;
    }
    return static_cast<double>(total) / static_cast<double>(document_lengths_.size());
}

std::uint64_t InvertedIndex::document_fingerprint(std::size_t doc_id) const {
    return doc_id < document_fingerprints_.size() ? document_fingerprints_[doc_id] : 0;
}

std::optional<std::size_t> InvertedIndex::find_document(std::string_view doc_id) const {
    for (std::size_t i = 0; i < document_ids_.size(); i++) {
        if (document_ids_[i] == doc_id) {
            return i;
        }
    }
    return std::nullopt;
}

std::vector<Token> InvertedIndex::document_terms(std::size_t doc_id) const {
    std::vector<Token> out;

    for (const auto& [term, postings] : postings_) {
        const auto it = std::lower_bound(
            postings.begin(), postings.end(), doc_id,
            [](const Posting& posting, std::size_t target) { return posting.doc_id < target; });
        if (it == postings.end() || it->doc_id != doc_id) {
            continue;
        }
        for (const std::size_t position : it->positions) {
            out.push_back(Token{term, position, 0});
        }
    }

    std::sort(out.begin(), out.end(),
              [](const Token& a, const Token& b) { return a.position < b.position; });
    return out;
}

const std::vector<std::string>& InvertedIndex::document_ids() const {
    return document_ids_;
}

std::vector<std::string> InvertedIndex::terms() const {
    std::vector<std::string> out;
    out.reserve(postings_.size());
    for (const auto& [term, postings] : postings_) {
        out.push_back(term);
    }
    std::sort(out.begin(), out.end());
    return out;
}

InvertedIndex InvertedIndex::from_parts(
    std::vector<std::string> document_ids,
    std::vector<std::size_t> document_lengths,
    std::vector<std::uint64_t> document_fingerprints,
    std::unordered_map<std::string, std::vector<Posting>> postings) {
    InvertedIndex index;
    index.document_ids_ = std::move(document_ids);
    index.document_lengths_ = std::move(document_lengths);
    index.document_fingerprints_ = std::move(document_fingerprints);
    index.postings_ = std::move(postings);
    return index;
}

std::uint64_t fingerprint_document(const std::filesystem::path& corpus_dir,
                                   const std::string& id) {
    std::ifstream in(corpus_dir / (id + ".txt"), std::ios::binary);
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

InvertedIndex update_index(const InvertedIndex& previous,
                           const std::filesystem::path& corpus_dir, IndexUpdateReport& report) {
    report = IndexUpdateReport{};

    const std::vector<std::string> ids = list_document_ids(corpus_dir);
    std::unordered_map<std::string, std::size_t> kept;

    InvertedIndex index;
    for (const std::string& id : ids) {
        const std::uint64_t fingerprint = fingerprint_document(corpus_dir, id);
        const std::optional<std::size_t> old = previous.find_document(id);

        if (old.has_value() && fingerprint != 0 &&
            previous.document_fingerprint(*old) == fingerprint) {
            index.add_document(id, previous.document_terms(*old), fingerprint);
            kept.emplace(id, *old);
            report.unchanged++;
            continue;
        }

        const std::optional<Document> doc = read_document(corpus_dir, id);
        if (!doc.has_value()) {
            continue;
        }

        index.add_document(id, analyze_document(*doc), fingerprint);
        if (old.has_value()) {
            report.updated++;
        } else {
            report.added++;
        }
    }

    for (const std::string& id : previous.document_ids()) {
        if (kept.find(id) == kept.end() &&
            std::find(ids.begin(), ids.end(), id) == ids.end()) {
            report.removed++;
        }
    }

    return index;
}

void InvertedIndex::append(InvertedIndex other) {
    const std::size_t offset = document_ids_.size();

    for (auto& [term, list] : other.postings_) {
        std::vector<Posting>& destination = postings_[term];
        destination.reserve(destination.size() + list.size());
        for (Posting& posting : list) {
            posting.doc_id += offset;
            destination.push_back(std::move(posting));
        }
    }

    document_ids_.insert(document_ids_.end(),
                         std::make_move_iterator(other.document_ids_.begin()),
                         std::make_move_iterator(other.document_ids_.end()));
    document_lengths_.insert(document_lengths_.end(), other.document_lengths_.begin(),
                             other.document_lengths_.end());
    document_fingerprints_.insert(document_fingerprints_.end(),
                                  other.document_fingerprints_.begin(),
                                  other.document_fingerprints_.end());
}

InvertedIndex build_index_from(const std::filesystem::path& corpus_dir,
                               const std::vector<std::string>& ids) {
    InvertedIndex index;
    for (const std::string& id : ids) {
        const std::optional<Document> doc = read_document(corpus_dir, id);
        if (doc.has_value()) {
            index.add_document(id, analyze_document(*doc), fingerprint_document(corpus_dir, id));
        }
    }
    return index;
}

InvertedIndex build_index(const std::filesystem::path& corpus_dir) {
    return build_index_from(corpus_dir, list_document_ids(corpus_dir));
}

InvertedIndex build_index_parallel(const std::filesystem::path& corpus_dir,
                                   std::size_t threads) {
    const std::vector<std::string> ids = list_document_ids(corpus_dir);
    if (threads <= 1 || ids.size() < 2) {
        return build_index_from(corpus_dir, ids);
    }

    const std::size_t slice_count = std::min(threads, ids.size());
    const std::size_t slice_size = (ids.size() + slice_count - 1) / slice_count;

    ThreadPool pool(slice_count);
    std::vector<std::future<InvertedIndex>> pending;
    pending.reserve(slice_count);

    for (std::size_t start = 0; start < ids.size(); start += slice_size) {
        const std::size_t end = std::min(start + slice_size, ids.size());
        std::vector<std::string> slice(ids.begin() + static_cast<std::ptrdiff_t>(start),
                                       ids.begin() + static_cast<std::ptrdiff_t>(end));
        pending.push_back(pool.submit(
            [corpus_dir, slice = std::move(slice)] { return build_index_from(corpus_dir, slice); }));
    }

    InvertedIndex index;
    for (std::future<InvertedIndex>& future : pending) {
        index.append(future.get());
    }
    return index;
}
