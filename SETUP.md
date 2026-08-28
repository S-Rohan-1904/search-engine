# Setting up

From a fresh clone to a working search engine over Wikipedia, and the
benchmarks that produced the numbers in [BENCHMARKS.md](BENCHMARKS.md).

Everything below takes about twenty minutes, most of it downloading Wikipedia.

---

## 1. What you need

| | |
|---|---|
| CMake | 3.20 or newer |
| A C++20 compiler | GCC 12+, Clang 15+, or Apple Clang 15+ |
| libcurl | optional — only the crawler needs it |
| Python 3 | 3.9+, for the test runner and the analytics script |

Without libcurl the build still succeeds; the crawler then works against
already-downloaded pages and cannot fetch over HTTP. Nothing else is affected.

macOS:

```bash
brew install cmake curl
```

Debian or Ubuntu:

```bash
sudo apt install cmake g++ libcurl4-openssl-dev python3
```

## 2. Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

You should get `build/search`. Check it:

```bash
./build/search
```

That prints every command. Nothing so far has touched the network or written
outside `build/`.

### If the build fails

**Undefined `std::__cxx11` symbols at link time, on macOS.** Your environment
is pointing the compiler at one standard library's headers while linking
another — usually a `CPLUS_INCLUDE_PATH` set for Homebrew GCC while CMake picks
Apple Clang. Build in a shell without it:

```bash
env -u CPLUS_INCLUDE_PATH cmake --build build -j
```

**A missing standard header such as `<filesystem>`, on macOS.** Some Command
Line Tools installs are missing `/Library/Developer/CommandLineTools/usr/include/c++/v1`.
Point CMake at a complete toolchain instead:

```bash
brew install llvm
cmake -S . -B build -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++
cmake --build build -j
```

Both of these look like project bugs and are not. Check the toolchain before
the code.

## 3. Run the tests

```bash
python3 tests/runner.py
```

```
All 872 checks passing.
```

These are end-to-end: each case runs the real binary and compares its output.
Nothing passes because a mock agreed with it. If this passes, the build is
good.

## 4. A first index, in ten seconds

The repository ships a 30-document corpus, so you can try everything before
downloading anything.

```bash
./build/search index-write corpus/fixtures fixtures.idx
./build/search query --snippet fixtures.idx 'cat'
./build/search repl fixtures.idx
```

In the REPL, `:help` lists what it understands. Queries take `AND`, `OR`,
`NOT`, parentheses and `"quoted phrases"`.

## 5. A real corpus: Simple English Wikipedia

About 600 MB downloaded, 400 MB after import, 188 MB as an index.

Wikipedia publishes CirrusSearch dumps — the same data its own search runs on,
with templates and markup already stripped. That is why these rather than
`pages-articles`: the wikitext stripper is the hard part of importing
Wikipedia, and it has been done properly upstream.

```bash
./tools/fetch_wikipedia.sh
```

Or by hand — dumps are kept for about three months, so check the index page at
`https://dumps.wikimedia.org/other/cirrussearch/` for a current date:

```bash
mkdir -p corpus/wikipedia && cd corpus/wikipedia
curl -O https://dumps.wikimedia.org/other/cirrussearch/current/simplewiki-20251229-cirrussearch-content.json.gz
```

Import it, then build the index:

```bash
gzcat corpus/wikipedia/simplewiki-*-cirrussearch-content.json.gz \
  | ./build/search wiki-import - corpus/wikipedia/simplewiki.corpus

./build/search index-write --threads 8 \
  corpus/wikipedia/simplewiki.corpus corpus/wikipedia/simplewiki.idx
```

That takes about 30 seconds on 8 cores and peaks around 2.9 GB. If that is more
memory than you want to give it, build in blocks instead — same output, byte
for byte, at whatever peak you choose:

```bash
./build/search index-build --block 5000 \
  corpus/wikipedia/simplewiki.corpus corpus/wikipedia/simplewiki.idx
```

`--block 5000` peaks at 0.72 GB. Smaller blocks use less and take slightly
longer.

Then:

```bash
./build/search query --snippet corpus/wikipedia/simplewiki.idx 'solar system'
```

That should come back in about 50 milliseconds.

### Query the index, not the corpus

```bash
./build/search query simplewiki.idx 'solar system'      # 50 ms
./build/search query simplewiki.corpus 'solar system'   # 40 seconds
```

Both work, and the second one rebuilds the entire index before answering.
Pass a `.corpus` file only when you mean to build; pass the `.idx` to search.

Snippets need the document text, which an index does not store. The corpus is
found automatically if it sits beside the index under the same name
(`simplewiki.idx` next to `simplewiki.corpus`), or you can say where it is with
`--corpus`.

## 6. Reproduce the benchmarks

```bash
./tools/bench.sh corpus/wikipedia/simplewiki.corpus
```

Prints the table in [BENCHMARKS.md](BENCHMARKS.md) — build times at one and all
threads, the phase breakdown, compression, query percentiles, and peak memory
for both build strategies. It also checks that the block build's output is
byte-identical to the in-memory build's, and says so in the table.

Takes about four minutes on the Wikipedia corpus. Try it on the fixtures first
to see the shape of the output:

```bash
./tools/bench.sh corpus/fixtures tests/fixtures/queries/basic.txt
```

Your numbers will differ from the published ones — different machine, different
dump. The relationships are what should hold: the block build should use a
fraction of the memory, and startup should be milliseconds rather than seconds.

## 7. Analyse a query set

```bash
./tools/analyze.py corpus/wikipedia/simplewiki.idx
./tools/analyze.py simplewiki.idx my_queries.txt --limit 20 --scorer tfidf
```

Where `bench.sh` answers "how fast", this answers "how fast, and what came
back": per-query latency, how many results each query found, which found
nothing, the score distribution, and the weakest top results.

It reports wall clock per invocation *including process startup*, which is what
you actually wait for at a terminal, and which is deliberately a different
number from the in-process figure `bench query` reports.

The query file is one query per line; `#` is not special, so anything on a line
is a query.

## 8. Measure relevance

The ranking is scored against [BEIR](https://github.com/beir-cellar/beir),
whose corpora, queries and relevance judgements are public and independent, and
which publishes BM25 baselines to compare against.

```bash
mkdir -p corpus/beir && cd corpus/beir
curl -O https://public.ukp.informatik.tu-darmstadt.de/thakur/BEIR/datasets/scifact.zip
unzip scifact.zip && cd ../..

./build/search beir-import corpus/beir/scifact/corpus.jsonl corpus/beir/scifact.corpus
./build/search index-write corpus/beir/scifact.corpus corpus/beir/scifact.idx
./build/search evaluate corpus/beir/scifact.idx \
  corpus/beir/scifact/queries.jsonl corpus/beir/scifact/qrels/test.tsv
```

```
queries 300
ndcg_at_10 0.6714
precision_at_10 0.0897
recall_at_100 0.9126
```

SciFact is 2.8 MB and takes under a minute end to end. The published BEIR BM25
baseline for it is 0.665.

TREC-COVID (`trec-covid.zip`, 74 MB, 171,331 documents) works the same way and
currently scores 0.3034 against a published baseline of 0.656. That gap is
unexplained; see [BENCHMARKS.md](BENCHMARKS.md).

## 9. Crawling

If libcurl was found at build time:

```bash
./build/search crawl https://example.com --max-pages 100 --out corpus/crawled
./build/search index-write corpus/crawled crawled.idx
```

The crawler reads `robots.txt` and obeys it, rate limits itself per host, and
drops pages whose content hashes to something already seen. Point it at your
own sites, or at ones whose terms permit it.

## Where things live

```
src/          the engine, one concern per file pair
tests/
  runner.py   the test runner
  cases/      one file per feature, each case an argv and expected output
  fixtures/   small corpora, indexes and query logs
tools/
  bench.sh          the benchmark table
  analyze.py        query-set analytics
  fetch_wikipedia.sh
corpus/
  fixtures/   the 30-document corpus, checked in
  wikipedia/  gitignored
  beir/       gitignored
```

Corpora and indexes are gitignored. Nothing large is in the repository, and
nothing in it needs the network to build or test.
