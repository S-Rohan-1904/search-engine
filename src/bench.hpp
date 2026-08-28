#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <vector>

#include "index.hpp"

// One measured figure.
//
// Measurements are split into two kinds because only one kind can be asserted
// on. `stable` values come from the data and are the same on every run and every
// machine; timings and memory are not, and a test that compared them would fail
// for reasons that have nothing to do with the code.
struct Measurement {
    std::string name;
    std::string value;
    bool stable;
};

// Peak resident set size of this process so far, in bytes.
//
// From getrusage rather than a guess or a sum of allocations, so it includes
// everything the process actually touched. The units differ by platform, which
// is the only reason this is a function rather than a one-liner at the call
// site.
std::uint64_t peak_memory_bytes();

// Total bytes of the corpus or index behind a source path.
std::uint64_t source_bytes(const std::filesystem::path& source);

// Measures building an index from `source`.
//
// Reports document, term and posting counts, the encoded index size, elapsed
// time, throughput in documents and bytes per second, and peak memory.
std::vector<Measurement> bench_build(const std::filesystem::path& source, std::size_t threads);

// Measures answering every query in `queries` against an already-open index.
//
// The index is opened by the caller and so is excluded from the timing, since
// the question is how fast queries are, not how fast startup is. It is the
// caller's job to reject an index that failed to open: benchmarking an empty
// one reports microsecond latencies for queries that matched nothing. Each query is run
// `repeats` times and the whole set is timed per query, so the percentiles
// describe individual queries rather than the batch.
//
// Reports the query count, total results returned, throughput, and the p50, p95
// and p99 latencies. Percentiles rather than a mean: a mean hides the tail, and
// the tail is what a user notices.
std::vector<Measurement> bench_query(const InvertedIndex& index,
                                     const std::vector<std::string>& queries,
                                     std::size_t limit, std::size_t repeats);

// Writes measurements as `name value` lines.
//
// `stable_only` drops everything that varies between runs, which is what makes
// the command testable and what makes two reports diffable.
void print_measurements(std::ostream& out, const std::vector<Measurement>& measurements,
                        bool stable_only);

// A header describing where the numbers came from.
//
// A measurement without its machine, compiler and corpus is not a measurement,
// so this is printed alongside every report rather than left to be remembered.
std::vector<Measurement> bench_environment(const std::filesystem::path& source);
