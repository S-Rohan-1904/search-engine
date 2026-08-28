#include "bench.hpp"

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <sys/resource.h>
#include <thread>

#include "index.hpp"
#include "index_io.hpp"
#include "results.hpp"

namespace {

std::string with_precision(double value, int digits) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(digits) << value;
    return out.str();
}

double elapsed_ms(std::chrono::steady_clock::time_point start,
                  std::chrono::steady_clock::time_point end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double percentile(std::vector<double> samples, double fraction) {
    if (samples.empty()) {
        return 0.0;
    }
    std::sort(samples.begin(), samples.end());

    const double position = fraction * static_cast<double>(samples.size() - 1);
    const std::size_t lower = static_cast<std::size_t>(position);
    const std::size_t upper = std::min(lower + 1, samples.size() - 1);
    const double weight = position - static_cast<double>(lower);

    return samples[lower] * (1.0 - weight) + samples[upper] * weight;
}

std::string compiler_name() {
#if defined(__clang__)
    return "clang " + std::to_string(__clang_major__) + "." + std::to_string(__clang_minor__);
#elif defined(__GNUC__)
    return "gcc " + std::to_string(__GNUC__) + "." + std::to_string(__GNUC_MINOR__);
#else
    return "unknown";
#endif
}

}

std::uint64_t peak_memory_bytes() {
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) != 0) {
        return 0;
    }

#if defined(__APPLE__)
    return static_cast<std::uint64_t>(usage.ru_maxrss);
#else
    return static_cast<std::uint64_t>(usage.ru_maxrss) * 1024;
#endif
}

std::uint64_t source_bytes(const std::filesystem::path& source) {
    std::error_code ec;

    if (std::filesystem::is_regular_file(source, ec)) {
        const auto size = std::filesystem::file_size(source, ec);
        return ec ? 0 : static_cast<std::uint64_t>(size);
    }

    if (!std::filesystem::is_directory(source, ec)) {
        return 0;
    }

    std::uint64_t total = 0;
    for (const auto& entry : std::filesystem::directory_iterator(source, ec)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::error_code size_ec;
            const auto size = std::filesystem::file_size(entry.path(), size_ec);
            if (!size_ec) {
                total += static_cast<std::uint64_t>(size);
            }
        }
    }
    return total;
}

std::vector<Measurement> bench_environment(const std::filesystem::path& source) {
    return {
        Measurement{"source", source.string(), true},
        Measurement{"source_bytes", std::to_string(source_bytes(source)), true},
        Measurement{"compiler", compiler_name(), false},
        Measurement{"hardware_threads", std::to_string(std::thread::hardware_concurrency()), false},
    };
}

std::vector<Measurement> bench_build(const std::filesystem::path& source, std::size_t threads) {
    const std::uint64_t bytes = source_bytes(source);

    BuildProfile profile;
    const auto start = std::chrono::steady_clock::now();
    const InvertedIndex index = build_index_parallel(source, threads, profile);
    const auto built = std::chrono::steady_clock::now();

    const std::vector<unsigned char> encoded = encode_index(index, IndexEncoding::VarByte);
    const auto encoded_at = std::chrono::steady_clock::now();

    const double build_ms = elapsed_ms(start, built);
    const double encode_ms = elapsed_ms(built, encoded_at);
    const double seconds = build_ms / 1000.0;

    std::vector<Measurement> out{
        Measurement{"threads", std::to_string(threads), true},
        Measurement{"documents", std::to_string(index.document_count()), true},
        Measurement{"terms", std::to_string(index.term_count()), true},
        Measurement{"postings", std::to_string(index.posting_count()), true},
        Measurement{"index_bytes", std::to_string(encoded.size()), true},
        Measurement{"build_ms", with_precision(build_ms, 1), false},
        Measurement{"open_ms", with_precision(profile.open_ms, 1), false},
        Measurement{"index_ms", with_precision(profile.index_ms, 1), false},
        Measurement{"merge_ms", with_precision(profile.merge_ms, 1), false},
        Measurement{"encode_ms", with_precision(encode_ms, 1), false},
    };

    if (seconds > 0.0) {
        out.push_back(Measurement{
            "documents_per_second",
            with_precision(static_cast<double>(index.document_count()) / seconds, 1), false});
        out.push_back(Measurement{
            "megabytes_per_second",
            with_precision(static_cast<double>(bytes) / seconds / 1048576.0, 2), false});
    }

    out.push_back(Measurement{"peak_memory_bytes", std::to_string(peak_memory_bytes()), false});
    return out;
}

std::vector<Measurement> bench_query(const InvertedIndex& index, const CorpusReader* corpus,
                                     const std::vector<std::string>& queries, std::size_t limit,
                                     std::size_t repeats, bool snippets) {
    QueryOptions options;
    options.limit = limit;
    options.snippets = snippets;

    std::vector<double> samples;
    samples.reserve(queries.size() * repeats);
    std::size_t results_total = 0;
    std::size_t failed = 0;

    for (std::size_t round = 0; round < repeats; round++) {
        for (const std::string& query : queries) {
            const auto start = std::chrono::steady_clock::now();
            std::string error;
            const std::optional<std::vector<QueryResult>> results =
                run_query(index, corpus, query, options, error);
            const auto end = std::chrono::steady_clock::now();

            samples.push_back(elapsed_ms(start, end));
            if (results.has_value()) {
                if (round == 0) {
                    results_total += results->size();
                }
            } else if (round == 0) {
                failed++;
            }
        }
    }

    double total_ms = 0.0;
    for (const double sample : samples) {
        total_ms += sample;
    }

    std::vector<Measurement> out{
        Measurement{"documents", std::to_string(index.document_count()), true},
        Measurement{"queries", std::to_string(queries.size()), true},
        Measurement{"repeats", std::to_string(repeats), true},
        Measurement{"limit", std::to_string(limit), true},
        Measurement{"snippets", snippets ? "on" : "off", true},
        Measurement{"results_returned", std::to_string(results_total), true},
        Measurement{"queries_rejected", std::to_string(failed), true},
    };

    if (!samples.empty()) {
        out.push_back(Measurement{"p50_ms", with_precision(percentile(samples, 0.50), 3), false});
        out.push_back(Measurement{"p95_ms", with_precision(percentile(samples, 0.95), 3), false});
        out.push_back(Measurement{"p99_ms", with_precision(percentile(samples, 0.99), 3), false});
        out.push_back(Measurement{"mean_ms",
                                  with_precision(total_ms / static_cast<double>(samples.size()), 3),
                                  false});
        if (total_ms > 0.0) {
            out.push_back(Measurement{
                "queries_per_second",
                with_precision(static_cast<double>(samples.size()) / (total_ms / 1000.0), 1),
                false});
        }
    }

    out.push_back(Measurement{"peak_memory_bytes", std::to_string(peak_memory_bytes()), false});
    return out;
}

void print_measurements(std::ostream& out, const std::vector<Measurement>& measurements,
                        bool stable_only) {
    for (const Measurement& measurement : measurements) {
        if (stable_only && !measurement.stable) {
            continue;
        }
        out << measurement.name << " " << measurement.value << "\n";
    }
}
