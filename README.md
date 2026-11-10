# search

A search engine written from scratch in C++20: crawler, indexer, ranker and
query language, with no search library underneath it. No Lucene, no Elastic,
no third-party index. The standard library, and libcurl for HTTP.

It indexes 278,214 Wikipedia documents in 9 seconds, answers a query in 1.4 ms,
and beats the published BM25 baselines on two public benchmarks: 0.6853 nDCG@10
on TREC-COVID against 0.656, and 0.6925 on SciFact against 0.665.

```console
$ search wiki-import simplewiki.json.gz simplewiki.corpus
$ search index-write --threads 8 simplewiki.corpus simplewiki.idx
$ search query --snippet simplewiki.idx 'solar system'
1. doc_00015284  15.9926
   ...Renewable energy Renewable resource [Solar] power "[Solar] [systems]
   projects"...
```

## Numbers

Full table, with the command that reproduces each figure, in
[BENCHMARKS.md](BENCHMARKS.md). Apple M2, 8 cores, warm page cache.

| | |
|---|---|
| Corpus | 278,214 docs, 1,204,562 terms, 26,162,711 postings |
| Index build | 8.7 s on 8 threads, 31,883 docs/s |
| Index size | 186 MB, 5.59x smaller than uncompressed |
| Startup | 35 ms, whole process, memory-mapped index |
| Query latency | p50 1.35 ms, p95 12.0 ms, p99 17.1 ms, 327 queries/s |
| With snippets | p50 2.64 ms, p95 14.0 ms |
| Memory, serving | 97 MB |
| Memory, building | 1.06 GB in blocks, 3.12 GB in memory |
| Relevance | nDCG@10 0.6853 TREC-COVID, 0.6925 SciFact (BEIR BM25: 0.656, 0.665) |
| Tests | 915 checks, zero warnings under `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion` |

## What it does

**Text processing.** Tokenizer, Unicode-aware normalization that folds Latin-1
and Latin Extended-A to ASCII (`Zürich` → `zurich`, `Straße` → `strasse`), a
full Porter stemmer implemented step by step, and stopword removal.

**Index.** Positional inverted index. Delta-encoded, variable-byte compressed,
5.46x smaller than a flat encoding. Built in parallel across threads with a
deterministic merge, giving the same bytes for any thread count. Incremental updates
re-analyze only documents whose content hash changed.

**Query language.** `AND`, `OR`, `NOT`, parentheses, and `"quoted phrases"`,
lexed and parsed into a boolean tree, evaluated over postings lists. Phrase
matching uses the stored token positions.

**Ranking.** TF-IDF, BM25, and BM25F with title matches weighted over body
matches, scored in parallel across document shards. PageRank over the crawled
link graph, per page or per site with each site's score given to its home page,
or a flat boost to every home page; any of them as a multiplier on the text
score.

**Crawler.** robots.txt parsing, a BFS frontier with URL normalization and
deduplication, path-prefix scoping, HTML-to-text extraction, rate limiting, and
content-hash duplicate detection, fetching from many hosts in parallel while
spacing requests to each one. Run for real against three documentation sites
(2,424 pages) and across 1,096 independent sites (2,994 pages).

**On top of that.** Snippet generation with query terms highlighted, spelling
correction via a BK-tree over Levenshtein distance ("did you mean"), trie
autocomplete, an interactive REPL, and `search serve`, a small HTTP front end
with a search page and a JSON API.

## The parts worth reading

**A memory-mapped index format.** Loading the index used to mean decoding all
25.6 million postings into the heap: 2.2 GB from a 188 MB file, 3.5 seconds
before the first query could run. The format now stores fixed-width tables for
the per-document arrays and a sorted table of term offsets, so opening it is a
`mmap` and a header read, finding a term is a binary search over mapped bytes,
and a postings list is decoded only when a query asks for it. **Startup went
from 3.48 s to 0.05 s and resident memory from 2.2 GB to 101 MB, with query
latency unchanged.**

**Building an index larger than memory.** The in-memory build peaks at 2.9 GB
for a 188 MB index, which caps corpus size for reasons that have nothing to do
with the disk. `index-build` indexes a block of documents at a time, writes
each block, and merges the blocks in one pass over sorted term streams. Peak
memory becomes a function of block size (0.72 GB at 5,000 documents per block),
and **the output is byte-identical to the in-memory build's at every block
size**, which is what makes the two interchangeable.

**Two quadratic bugs that only a real corpus exposes.** Both were invisible on
a 30-document test corpus. One made indexing 39 billion string comparisons; one
made a single query O(postings × documents) by re-summing every document length
inside the BM25 inner loop. Ten queries ran past ten minutes. They are the
reason the build takes seconds and the query milliseconds.

**Skip pointers, measured and rejected.** Galloping search in postings
intersection, the textbook optimisation, made intersection-heavy queries
*twice as slow*. The scan was never the cost; decoding the postings was. The
attempt and the reason are kept rather than quietly dropped.

## Measuring relevance honestly

Precision measured against judgements written for the engine being measured is
not evidence of anything. This is scored against [BEIR](https://github.com/beir-cellar/beir),
whose corpora, queries and relevance judgements are public, independent, and
come with published BM25 baselines.

| Dataset | Documents | Queries | nDCG@10 | Published BEIR BM25 |
|---|---|---|---|---|
| SciFact | 5,183 | 300 | 0.6925 | 0.665 |
| TREC-COVID | 171,331 | 50 | 0.6853 | 0.656 |

Both with `--scorer bm25f`. TREC-COVID first measured 0.3034. Finding out why
took an independent BM25 in Python to show the gap was in this engine, then one
suspect changed at a time: the benchmark's queries were split differently from
the documents, so "COVID-19" never matched; hyphenated terms like `sars-cov-2`
were kept whole; and nDCG used exponential gain where `trec_eval` uses linear.
The engine's own nDCG agrees with `pytrec_eval` on its run file to four
places. The whole account is in [BENCHMARKS.md](BENCHMARKS.md).

**PageRank, measured on three kinds of graph.** On a documentation crawl it
lowered MRR@10 at every weight: inside a site the best-linked pages are the
indexes in every navigation bar. On Wikipedia's 12.6 million links it raised
navigational queries by 0.035 and lowered lookups of a particular article by
0.007, both with intervals clear of zero, so the cost is PageRank's own. On a
crawl of 1,096 sites, ranking sites instead of pages (`--pagerank-graph site`)
raised homepage finding from 0.565 to 0.775, and applying it only to one- and
two-word queries (`--pagerank-max-words 2`) kept two thirds of that gain while
cutting the cost to 0.004. A control then gave every home page the same boost,
with no links at all (`--pagerank-graph home`): that reached 0.851, so most of
the site-level gain is preferring home pages, and the links add 0.02 to 0.03
over chance.

## Building

Needs CMake 3.20+, a C++20 compiler, and Python 3. libcurl is optional and only
the crawler uses it.

```console
$ cmake -S . -B build && cmake --build build -j
$ python3 tests/runner.py
All 957 checks passing.
```

Tests are end-to-end: each case runs the real binary and compares its output,
so nothing passes because a mock agreed with it.

[SETUP.md](SETUP.md) goes from a fresh clone to a Wikipedia index and the
benchmarks, in about twenty minutes.

Reproduce the numbers, or look at what a query set actually returns:

```console
$ ./tools/bench.sh corpus/wikipedia/simplewiki.corpus
$ ./tools/analyze.py corpus/wikipedia/simplewiki.idx
```

All commands: [COMMANDS.md](COMMANDS.md).

## Commands

```
search index-write [--threads n] <source> <file>   build and save an index
search index-build [--block n] <source> <file>     build one larger than memory
search index-update <corpus> <file>                re-index only what changed
search query [options] <source> <query>            ranked results
search repl [options] <source>                     interactive queries
search match <source> <query>                      boolean matching only
search evaluate <index> <queries> <qrels>          nDCG@10, P@10, recall@100
search bench build|query <source> [...]            measure it yourself
search index-size <source>                         compression breakdown
search crawl <url> [...]                           fetch a site into a corpus
search serve [--port n] <source>                   search from a browser
search wiki-import <dump> <file>                   corpus from a Wikipedia dump
search beir-import <corpus.jsonl> <file>           corpus from a BEIR dataset
```

`search` with no arguments lists all of them, including the smaller tools for
inspecting tokenization, stemming, postings and parse trees.
