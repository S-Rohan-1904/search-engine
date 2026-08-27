# search-engine

A full-text search engine written from scratch in C++20: inverted index,
positional postings, boolean and phrase queries, BM25 ranking, a compressed
on-disk index, and parallel index construction. No search or IR libraries; the
tokenizer, stemmer, index, compression codecs and ranking functions are all
implemented here.

## Building

Requires CMake ≥ 3.20 and a C++20 compiler.

```console
$ cmake -S . -B build
$ cmake --build build -j
```

The binary lands at `build/search`.

## Usage

```console
$ ./build/search docs corpus/fixtures
doc_001
doc_002
...

$ ./build/search analyze "The cats were running quickly"
1 4 cat
3 14 run
4 22 quickli
```

`analyze` runs the full analysis chain (tokenize, normalize, drop stopwords,
stem) and prints one line per surviving term: its position in the token stream,
its byte offset in the source, and the term itself. Positions are not
renumbered when a token is filtered out, so the gaps above are real and phrase
matching can rely on them.

`analyze-doc <corpus_dir> <doc_id>` does the same for a document on disk.

More commands land as the engine grows; see the roadmap below.

## Tests

End-to-end tests drive the compiled binary from the command line and check its
stdout and exit codes, so the internals stay free to change.

```console
$ ./run_tests.sh           # build, then run every suite
$ ./run_tests.sh corpus    # only suites matching "corpus"
$ ./run_tests.sh --list    # list the suites
```

Each suite in `tests/cases/` is a plain Python list of cases: an argv, an
expected stdout, an expected exit code.

## Corpus

`corpus/fixtures/` holds 30 short hand-written documents across four loosely
overlapping topics (animals, programming, cooking, space). They are deliberately
small, small enough to compute a BM25 score by hand and check it against what
the engine reports. Larger corpora (a Wikipedia abstracts subset) get wired in
once the on-disk index exists and throughput starts to matter.

Document format:

```
title: Cats and Mats

The cat sat on the mat. ...
```

## Layout

```
src/                 engine source
tests/runner.py      the end-to-end harness
tests/cases/         one suite per area of behaviour
tests/fixtures/      small corpora used only by tests
corpus/fixtures/     the working corpus
```

## Roadmap

- [x] Corpus ingestion and document listing
- [x] Tokenizer: case folding, punctuation, numbers, hyphens
- [x] Stopword filtering
- [x] Porter stemmer
- [ ] Inverted index with term frequencies and positions
- [ ] Query parsing to a boolean AST
- [ ] Boolean retrieval: AND / OR / NOT over postings lists
- [ ] Phrase queries via positional intersection
- [ ] TF-IDF, then BM25, with top-K selection through a min-heap
- [ ] Binary on-disk index with delta + variable-byte encoded postings
- [ ] Parallel index construction with a deterministic merge
- [ ] Snippet generation
- [ ] Spelling correction (edit distance over a BK-tree)
- [ ] Trie-backed autocomplete
- [ ] Incremental indexing
- [ ] A crawler for building corpora from the web
