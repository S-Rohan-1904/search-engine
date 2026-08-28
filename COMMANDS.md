# Commands

Quick reference. Full walkthrough in [SETUP.md](SETUP.md).

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

## Test

```bash
python3 tests/runner.py
```

## Benchmark

```bash
./tools/bench.sh <corpus> [query_file]

./tools/bench.sh corpus/fixtures tests/fixtures/queries/basic.txt   # ~5s, sanity check
./tools/bench.sh corpus/wikipedia/simplewiki.corpus                 # ~4 min, full table
```

Prints the BENCHMARKS.md table: build time (1 thread / all threads), phase
breakdown, compression, query p50/p95/p99, peak memory for both build
strategies, byte-identity check.

## Analyze a query set

```bash
./tools/analyze.py <index> [query_file] [--limit N] [--repeats N] [--scorer bm25|tfidf]

./tools/analyze.py corpus/wikipedia/simplewiki.idx
./tools/analyze.py corpus/wikipedia/simplewiki.idx my_queries.txt --limit 20
```

Per-query latency and result counts, which queries returned nothing, score
distribution, latency histogram. Needs a built index (`.idx`), not a `.corpus`
file — pass the corpus and it will just answer slowly by rebuilding first.

## Build an index

```bash
./build/search index-write --threads 8 <corpus> <index.idx>          # in memory, fast
./build/search index-build --block 5000 <corpus> <index.idx>         # bounded memory
```

## Query

```bash
./build/search query --snippet <index.idx> 'solar system'
./build/search repl <index.idx>
```

## Relevance (BEIR)

```bash
./build/search beir-import <corpus.jsonl> <out.corpus>
./build/search evaluate <index.idx> <queries.jsonl> <qrels.tsv>
```
