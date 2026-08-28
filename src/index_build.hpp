#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

// Building an index that does not have to fit in memory.
//
// The in-memory build holds every posting for the whole corpus before it writes
// anything: 278,214 Wikipedia documents peak at 2.9 GB for an index that is
// 188 MB on disk. The multiplier is the in-memory shape, not the data, and it
// puts a ceiling on corpus size that has nothing to do with the disk.
//
// The classic answer, and the one here: index a block of documents at a time,
// write each block out, and merge the blocks. Peak memory becomes a function of
// the block size rather than of the corpus, so a corpus larger than memory is
// only slower, not impossible.
//
// The merge is what makes it cheap. Every block file stores its terms sorted,
// so merging them is one pass over several sorted streams, holding one term's
// postings at a time. Nothing is ever fully in memory again.
struct ExternalBuildReport {
    std::size_t documents = 0;
    std::size_t terms = 0;
    std::size_t postings = 0;
    std::size_t blocks = 0;
    double index_ms = 0.0;  // indexing the blocks
    double merge_ms = 0.0;  // merging them into the output
};

// Builds an index over `source` into `output`, never holding more than
// `block_documents` documents' postings at once.
//
// The result is byte-identical to what an in-memory build of the same corpus
// writes, for any block size, which is what makes the two interchangeable.
bool build_index_external(const std::filesystem::path& source,
                          const std::filesystem::path& output, std::size_t block_documents,
                          std::size_t threads, ExternalBuildReport& report, std::string& error);
