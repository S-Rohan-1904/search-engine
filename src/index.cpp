#include "index.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <future>
#include <iterator>
#include <optional>
#include <unordered_set>
#include <string>
#include <utility>

#include "analyzer.hpp"
#include "corpus.hpp"
#include "index_map.hpp"
#include "document.hpp"
#include "thread_pool.hpp"

struct InvertedIndex::Backing {
    explicit Backing(MappedIndex index) : mapped(std::move(index)) {}

    MappedIndex mapped;
    std::mutex mutex;
    std::unordered_map<std::string, std::vector<Posting>> decoded;
};

InvertedIndex InvertedIndex::from_mapped(MappedIndex mapped) {
    InvertedIndex index;
    index.mapped_ = std::make_shared<Backing>(std::move(mapped));
    return index;
}

void InvertedIndex::add_document(std::string doc_id, const std::vector<Token>& terms,
                                 std::uint64_t fingerprint) {
    const std::size_t ordinal = document_ids_.size();
    document_ids_.push_back(std::move(doc_id));
    document_lengths_.push_back(terms.size());
    total_document_length_ += terms.size();
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
    return mapped_ ? mapped_->mapped.document_count() : document_ids_.size();
}

std::size_t InvertedIndex::term_count() const {
    return mapped_ ? mapped_->mapped.term_count() : postings_.size();
}

std::size_t InvertedIndex::posting_count() const {
    if (mapped_) {
        return mapped_->mapped.posting_count();
    }

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

    if (mapped_) {
        const std::lock_guard<std::mutex> guard(mapped_->mutex);
        const auto cached = mapped_->decoded.find(std::string(term));
        if (cached != mapped_->decoded.end()) {
            return cached->second;
        }
        return mapped_->decoded.emplace(std::string(term), mapped_->mapped.postings(term))
            .first->second;
    }

    const auto it = postings_.find(std::string(term));
    return it == postings_.end() ? kEmpty : it->second;
}

std::size_t InvertedIndex::document_length(std::size_t doc_id) const {
    if (mapped_) {
        return mapped_->mapped.document_length(doc_id);
    }
    return doc_id < document_lengths_.size() ? document_lengths_[doc_id] : 0;
}

double InvertedIndex::average_document_length() const {
    if (mapped_) {
        const std::size_t documents = mapped_->mapped.document_count();
        if (documents == 0) {
            return 0.0;
        }
        return static_cast<double>(mapped_->mapped.total_document_length()) /
               static_cast<double>(documents);
    }

    if (document_lengths_.empty()) {
        return 0.0;
    }

    return static_cast<double>(total_document_length_) /
           static_cast<double>(document_lengths_.size());
}

std::uint64_t InvertedIndex::document_fingerprint(std::size_t doc_id) const {
    if (mapped_) {
        return mapped_->mapped.document_fingerprint(doc_id);
    }
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
    if (mapped_) {
        return mapped_->mapped.document_terms(doc_id);
    }

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
    // The mapped file stores ids as bytes, and a caller that wants the whole
    // list wants them as strings. Building that list is the one thing here
    // that is proportional to the corpus, so it happens on demand rather than
    // at open time: a query prints ten ids and never asks for the rest.
    if (mapped_ && document_ids_.size() != mapped_->mapped.document_count()) {
        const std::lock_guard<std::mutex> guard(mapped_->mutex);
        if (document_ids_.size() != mapped_->mapped.document_count()) {
            const std::size_t documents = mapped_->mapped.document_count();
            document_ids_.clear();
            document_ids_.reserve(documents);
            for (std::size_t doc_id = 0; doc_id < documents; doc_id++) {
                document_ids_.emplace_back(mapped_->mapped.document_id(doc_id));
            }
        }
    }
    return document_ids_;
}

std::vector<std::string> InvertedIndex::terms() const {
    if (mapped_) {
        return mapped_->mapped.terms();
    }

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
    for (const std::size_t length : index.document_lengths_) {
        index.total_document_length_ += length;
    }
    return index;
}

InvertedIndex update_index(const InvertedIndex& previous,
                           const std::filesystem::path& corpus_dir, IndexUpdateReport& report) {
    report = IndexUpdateReport{};

    std::string error;
    const std::unique_ptr<CorpusReader> corpus = open_corpus(corpus_dir, error);
    if (!corpus) {
        return InvertedIndex{};
    }

    const std::vector<std::string> ids = corpus->document_ids();

    std::unordered_map<std::string, std::size_t> previous_of;
    previous_of.reserve(previous.document_ids().size());
    for (std::size_t i = 0; i < previous.document_ids().size(); i++) {
        previous_of.emplace(previous.document_ids()[i], i);
    }

    std::unordered_map<std::string, std::size_t> kept;

    InvertedIndex index;
    for (const std::string& id : ids) {
        const std::uint64_t fingerprint = corpus->fingerprint(id);

        const auto found = previous_of.find(id);
        const std::optional<std::size_t> old =
            found == previous_of.end() ? std::nullopt : std::optional<std::size_t>(found->second);

        if (old.has_value() && fingerprint != 0 &&
            previous.document_fingerprint(*old) == fingerprint) {
            index.add_document(id, previous.document_terms(*old), fingerprint);
            kept.emplace(id, *old);
            report.unchanged++;
            continue;
        }

        const std::optional<Document> doc = corpus->read(id);
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

    std::unordered_set<std::string> current(ids.begin(), ids.end());
    for (const std::string& id : previous.document_ids()) {
        if (kept.find(id) == kept.end() && current.find(id) == current.end()) {
            report.removed++;
        }
    }

    return index;
}

void InvertedIndex::append(InvertedIndex other) {
    const std::size_t offset = document_ids_.size();

    // Merging is serial, so it bounds how much a parallel build can win. Two
    // things keep it off the critical path: reserving once, so a merge of a
    // million terms does not rehash on the way, and moving a whole postings
    // list across when the term is new here, rather than moving its postings
    // one at a time into a freshly grown vector.
    postings_.reserve(postings_.size() + other.postings_.size());

    for (auto& [term, list] : other.postings_) {
        const auto found = postings_.find(term);
        if (found == postings_.end()) {
            for (Posting& posting : list) {
                posting.doc_id += offset;
            }
            postings_.emplace(term, std::move(list));
            continue;
        }

        std::vector<Posting>& destination = found->second;
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
    total_document_length_ += other.total_document_length_;
    document_fingerprints_.insert(document_fingerprints_.end(),
                                  other.document_fingerprints_.begin(),
                                  other.document_fingerprints_.end());
}

InvertedIndex build_index_from(const CorpusReader& corpus,
                               const std::vector<std::string>& ids) {
    InvertedIndex index;
    for (const std::string& id : ids) {
        const std::optional<Document> doc = corpus.read(id);
        if (doc.has_value()) {
            index.add_document(id, analyze_document(*doc), corpus.fingerprint(id));
        }
    }
    return index;
}

InvertedIndex build_index(const std::filesystem::path& corpus_dir) {
    std::string error;
    const std::unique_ptr<CorpusReader> corpus = open_corpus(corpus_dir, error);
    if (!corpus) {
        return InvertedIndex{};
    }
    return build_index_from(*corpus, corpus->document_ids());
}

InvertedIndex build_index_parallel(const std::filesystem::path& corpus_dir,
                                   std::size_t threads) {
    BuildProfile ignored;
    return build_index_parallel(corpus_dir, threads, ignored);
}

InvertedIndex build_index_parallel(const std::filesystem::path& corpus_dir, std::size_t threads,
                                   BuildProfile& profile) {
    using Clock = std::chrono::steady_clock;
    const auto ms = [](Clock::time_point from, Clock::time_point to) {
        return std::chrono::duration<double, std::milli>(to - from).count();
    };

    profile = BuildProfile{};
    const auto opened_at = Clock::now();

    std::string error;
    const std::unique_ptr<CorpusReader> corpus = open_corpus(corpus_dir, error);
    if (!corpus) {
        return InvertedIndex{};
    }

    const std::vector<std::string> ids = corpus->document_ids();
    const auto listed_at = Clock::now();
    profile.open_ms = ms(opened_at, listed_at);

    if (threads <= 1 || ids.size() < 2) {
        InvertedIndex index = build_index_from(*corpus, ids);
        profile.index_ms = ms(listed_at, Clock::now());
        return index;
    }

    const std::size_t slice_count = std::min(threads, ids.size());
    const std::size_t slice_size = (ids.size() + slice_count - 1) / slice_count;

    ThreadPool pool(threads);
    std::vector<std::future<InvertedIndex>> pending;
    pending.reserve(slice_count);

    for (std::size_t start = 0; start < ids.size(); start += slice_size) {
        const std::size_t end = std::min(start + slice_size, ids.size());
        std::vector<std::string> slice(ids.begin() + static_cast<std::ptrdiff_t>(start),
                                       ids.begin() + static_cast<std::ptrdiff_t>(end));
        pending.push_back(pool.submit([&corpus, slice = std::move(slice)] {
            return build_index_from(*corpus, slice);
        }));
    }

    std::vector<InvertedIndex> slices;
    slices.reserve(pending.size());
    for (std::future<InvertedIndex>& future : pending) {
        slices.push_back(future.get());
    }

    const auto indexed_at = Clock::now();
    profile.index_ms = ms(listed_at, indexed_at);

    InvertedIndex index;
    for (InvertedIndex& slice : slices) {
        index.append(std::move(slice));
    }

    profile.merge_ms = ms(indexed_at, Clock::now());
    return index;
}
