# Benchmarks

Every figure here comes from a command in this repository, and the command is
printed beside it. A number without a way to reproduce it is not a measurement.

**Machine:** Apple M2 (4 performance + 4 efficiency cores), macOS, gcc 15.1,
`-O2`, `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`, no
warnings.

**Corpus:** Simple English Wikipedia, CirrusSearch dump of 2025-12-29, imported
with `search wiki-import`.

    documents        278,214
    distinct terms 1,443,435
    postings      25,643,127
    corpus bytes 424,874,389

Query timings are warm: the page cache holds the index. Cold runs are several
times slower and are not what an interactive tool does after its first query.

---

## Index construction

| Measurement      | Value                    | Reproduce with                            |
| ---------------- | ------------------------ | ----------------------------------------- |
| Build, 1 thread  | 40.9 s                   | `search bench build --threads 1 <corpus>` |
| Build, 8 threads | 15.9 s                   | `search bench build --threads 8 <corpus>` |
| Parallel speedup | 2.54x                    | the two rows above                        |
| Ingest rate      | 15,435 docs/s, 22.5 MB/s | `search bench build --threads 8 <corpus>` |
| Corpus open      | 41 ms                    | `open_ms` in the same output              |

`bench build` reports the build in three phases — `open_ms`, `index_ms`,
`merge_ms` — because a single total cannot say where a speedup stops. At 8
threads the serial merge is about 3.4 s of the 15.9 s, which is what caps the
speedup at 2.54x rather than anything about the parallel phase.

## Index size

| Measurement           | Value               | Reproduce with              |
| --------------------- | ------------------- | --------------------------- |
| Uncompressed          | 1,024,973,539 bytes | `search index-size <index>` |
| Delta + variable-byte | 187,589,343 bytes   | same                        |
| Compression ratio     | 5.46x               | same                        |

## Query latency

25 queries — keyword, boolean, and phrase — 5 repeats each.

| Measurement                            | Value         | Reproduce with                                     |
| -------------------------------------- | ------------- | -------------------------------------------------- |
| Startup, one query, total process time | 50 ms         | `time search query <index> 'solar system'`         |
| p50                                    | 4.09 ms       | `search bench query --repeats 5 <index> <queries>` |
| p95                                    | 33.8 ms       | same                                               |
| p99                                    | 44.5 ms       | same                                               |
| Throughput                             | 113 queries/s | same                                               |
| p50, with snippets                     | 6.43 ms       | add `--snippet`                                    |
| p95, with snippets                     | 42.3 ms       | same                                               |
| Peak memory, serving                   | 101 MB        | `/usr/bin/time -l` on the above                    |

## Memory during construction

| Build                       | Blocks | Peak memory | Wall clock |
| --------------------------- | ------ | ----------- | ---------- |
| In memory, 8 threads        | 1      | 2.92 GB     | 15.9 s     |
| In memory, 1 thread         | 1      | 2.53 GB     | 40.9 s     |
| `index-build --block 25000` | 12     | 1.01 GB     | 59.7 s     |
| `index-build --block 5000`  | 56     | 0.72 GB     | 66.2 s     |

The block builds produce a file byte-identical to the in-memory build's, at
every block size tested, so the block size is a memory setting and not a
correctness question.

## Relevance

Measured against BEIR, whose corpora, queries and relevance judgements are
public and independent of this code, and whose BM25 baselines are published per
dataset.

| Dataset    | Documents | Queries | nDCG@10 | Published BEIR BM25 |
| ---------- | --------- | ------- | ------- | ------------------- |
| SciFact    | 5,183     | 300     | 0.6714  | 0.665               |
| TREC-COVID | 171,331   | 50      | 0.3034  | 0.656               |

    search evaluate <index> <queries.jsonl> <qrels.tsv>

SciFact matches the published baseline. **TREC-COVID does not, and the reason
is not yet known.** The BM25 parameters have been ruled out: running with
Anserini's k1=0.9 and b=0.4 instead of 1.2 and 0.75 moves TREC-COVID only to
0.3326 and drops SciFact to 0.6636. Analysis differences on long medical
abstracts, and the handling of long queries, remain untested.

The honest claim from this is the narrow one: the BM25 implementation matches a
published Lucene baseline on SciFact. Not that it matches Lucene generally.

## Two quadratic bugs, found by running against a real corpus

Both were invisible on the 30-document test corpus and obvious at 278k.

**Build.** `CorpusFile::read` and `fingerprint` did a linear scan over all ids,
once per document. Indexing was about 39 billion string comparisons; it ran for
over four minutes before being killed. Fixed with a permutation sorted by id
and a binary search: **the build now finishes in 15.9 s.**

**Query.** `bm25_weight` called `average_document_length()` once per posting
scored, and that function re-summed all 278,214 document lengths on every call,
making one query O(postings × documents). Ten queries ran past ten minutes.
Fixed with a running total: **p50 is now 4.09 ms.**

## A benchmark that was lying

`bench query` handled a corpus _directory_ and an index file, and silently fell
through for a `.corpus` file: the load failed, the failure was swallowed, and
it benchmarked an empty index. It reported **0.004 ms p50 and 231,481
queries/s** over 25 queries that returned nothing, and exited 0.

It now opens its source the way every other command does, refuses an index with
no documents, and reports a `documents` line so a report that measured nothing
says so on its face.

## Skip pointers: measured and rejected

Galloping search in postings intersection, the expected optimisation, made
intersection-heavy queries **twice as slow** (96.3 → 52.6 queries/s). The scan
was never the cost; decoding the postings was.
