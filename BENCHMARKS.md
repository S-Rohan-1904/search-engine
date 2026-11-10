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
| SciFact    | 5,183     | 300     | 0.6845  | 0.665               |
| TREC-COVID | 171,331   | 50      | 0.6632  | 0.656               |

    search evaluate <index> <queries.jsonl> <qrels.tsv>

The engine's nDCG@10 was cross-checked by writing its top 100 per query to a
run file and scoring that with `pytrec_eval`, the scorer BEIR itself uses:
0.6631 against the engine's 0.6632, the difference being how ties are ordered.

### How TREC-COVID went from 0.3034 to 0.6632

TREC-COVID first measured 0.3034, less than half the baseline, while SciFact
matched. An independent BM25 written in Python (Lucene-style tokenization,
NLTK's Porter stemmer, the same k1 and b) scored 0.6101 on the same files,
which put the gap in this engine rather than in BM25. Each suspect was then
changed on its own and measured:

| Change                                                      | TREC-COVID | SciFact |
| ----------------------------------------------------------- | ---------- | ------- |
| Starting point                                              | 0.3034     | 0.6711  |
| Benchmark queries split on whitespace, as documents are     | 0.5872     | 0.6711  |
| Hyphens, slashes and Unicode dashes split compounds         | 0.6418     | 0.6845  |
| nDCG gain is the grade itself, as `trec_eval` computes it   | 0.6632     | 0.6845  |

1. **Query handling.** The evaluator turned every non-alphanumeric byte of a
   query into a space, so "COVID-19" became the two terms `covid` and `19`,
   while the documents had indexed the single term `covid-19`. The term that
   appears in nearly every TREC-COVID query matched nothing. Queries now go
   through the same whitespace tokenizer as documents.
2. **Tokenization of medical text.** Keeping hyphenated words whole made
   `sars-cov-2`, `covid-19` and `2019-ncov` single terms, so a document
   writing "SARS-CoV-2" never matched a query about "SARS CoV 2" or "CoV", and
   "ACE2/TMPRSS2" became one term `ace2tmprss2`. The analyzer now splits at
   hyphens, slashes and the Unicode hyphen and dash characters, the way
   Lucene's standard tokenizer does. The Python reference predicted this: with
   compounds kept whole it dropped from 0.6101 to 0.5603.
3. **The metric.** nDCG used a gain of 2^grade - 1. `trec_eval` and so BEIR
   use the grade itself. SciFact's grades are all 1, where the two agree,
   which is why only TREC-COVID moved.

Long-query handling and title-plus-abstract indexing were tested in the Python
reference rather than assumed. The engine's 153-word stopword list, against
Lucene's 33, is worth +0.053 on TREC-COVID because its queries are questions
("what is the origin of COVID-19"); the engine already had it. Scoring title
and abstract as separate fields and combining them the way BEIR's
Elasticsearch baseline does (`best_fields`, tie breaker 0.5) is worth a
further +0.073 in the reference, and is what the BM25F weighting below
addresses. Anserini's k1=0.9 and b=0.4 lower the score in both.

## Skip pointers: measured and rejected

Galloping search in postings intersection, the expected optimisation, made
intersection-heavy queries **twice as slow** (96.3 → 52.6 queries/s). The scan
was never the cost; decoding the postings was.
