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

Most commands take a *source*, which is either a corpus directory or a saved
index file. The engine builds or loads whichever it is given.

### Searching

```console
$ ./build/search match corpus/fixtures 'cat AND NOT (mat OR sat)'
doc_002
doc_003
doc_004

$ ./build/search phrase corpus/fixtures "sun is a star"
doc_026

$ ./build/search top --bm25 corpus/fixtures 3 cat mat
doc_001 7.4906
doc_003 2.5076
doc_004 2.1294
```

`match` accepts `AND`, `OR`, `NOT`, parentheses and `"quoted phrases"`.
Operators are uppercase, so `rock and roll` searches for three words rather
than a boolean. Two adjacent words mean AND.

Phrase matching compares positions, not adjacent words, so the stopwords a
phrase contains still occupy their slots: `sun is a star` requires a gap of
three and `sun star` does not match it.

### Inspecting the pipeline

```console
$ ./build/search analyze "The cats were running quickly"
1 4 cat
3 14 run
4 22 quickli
```

`analyze` runs the full chain (tokenize, normalize, drop stopwords, stem) and
prints one line per surviving term: its position in the token stream, its byte
offset in the source, and the term itself. Positions are never renumbered when
a token is filtered out, so the gaps above are real, and phrase matching relies
on them.

`tokenize`, `normalize`, `terms`, `stopword`, `stem` and `stem-step` expose the
individual stages; `analyze-doc` runs the chain over a document on disk.

### Inspecting the index

```console
$ ./build/search index-stats corpus/fixtures
documents: 30
terms: 235
postings: 343

$ ./build/search positions corpus/fixtures cat
doc_001 0 4 10 24
doc_002 15
doc_003 2 5 13 18 24
```

`index-terms` lists every term with its document frequency, `postings` and `tf`
give the documents and counts for one term, and `lengths` reports document
lengths and the corpus average.

### Saving and loading

```console
$ ./build/search index-write corpus/fixtures index.bin
$ ./build/search match index.bin 'cat AND mat'
doc_001
```

`--encoding plain|delta|varbyte` selects the integer layout, and `index-size`
reports what each one costs:

```console
$ ./build/search index-size corpus/fixtures
plain 17570 header 9 documents 698 dictionary 4895 postings 8232 positions 3736
delta 17570 header 9 documents 698 dictionary 4895 postings 8232 positions 3736
varbyte 3375 header 9 documents 271 dictionary 1599 postings 1029 positions 467
ratio 5.21
```

Delta encoding alone changes nothing, because every number is still eight fixed
bytes; it exists to make the values small so variable-byte encoding has
something to exploit. Together they are 5.2x smaller here, and the breakdown
shows why: postings and positions compress eightfold, while the dictionary
manages 3.1x because most of its bytes are the term strings themselves.

### Crawling

```console
$ ./build/search crawl --mirror tests/fixtures/site http://example.com/
0 200 http://example.com/
1 200 http://example.com/a.html
1 200 http://example.com/b.html
1 200 http://example.com/deep/c.html
1 200 http://example.com/private/public.html

$ ./build/search crawl --out crawled https://example.org/
$ ./build/search bm25 crawled cats
```

The crawler is breadth first, obeys robots.txt including `Crawl-delay`, spaces
its requests per host, and drops pages whose bytes it has already seen. `--out`
writes each page as a corpus document, so a crawl feeds straight into the index.

`--mirror <dir>` serves a site from the filesystem instead of the network, which
is how the tests exercise the crawler without depending on anything staying up.
HTTP fetching uses libcurl when the build found it; without curl the mirror path
still works.

`url`, `url-resolve`, `robots`, `crawl-delay`, `html-text` and `html-links`
expose the individual pieces.

### Threads

```console
$ ./build/search index-write --threads 8 corpus/fixtures index.bin
$ ./build/search bm25 --threads 8 corpus/fixtures cat mat
```

`--threads` parallelizes index construction and scoring. Results are identical
at any thread count, down to the bytes of the index file: slices are merged in
order, and scoring shards by document rather than by term so that no
floating-point sum is ever regrouped.

Run `./build/search` with no arguments for the full command list.

## Tests

End-to-end tests drive the compiled binary from the command line and check its
stdout and exit codes, so the internals stay free to change.

```console
$ ./run_tests.sh           # build, then run every suite
$ ./run_tests.sh phrase    # only suites matching "phrase"
$ ./run_tests.sh --list    # list the suites
```

Each suite in `tests/cases/` is a plain Python list of cases: an argv, an
expected stdout, an expected exit code. There are 561 checks across 33 suites.

Expected values for the parts that are easy to get subtly wrong were not written
by hand. The stemmer was checked against a second implementation over 20,000
dictionary words and against every worked example in Porter's paper; query
evaluation and ranking against independently written reference implementations
over randomly generated queries; the on-disk format by comparing every command
between a directory-built index and a file-loaded one across all three
encodings. The threading is checked by running each command at seven different
thread counts and hashing the results, and under ThreadSanitizer.

## Corpus

`corpus/fixtures/` holds 30 short hand-written documents across four loosely
overlapping topics (animals, programming, cooking, space). They are deliberately
small, small enough to compute a BM25 score by hand and check it against what
the engine reports. Larger corpora (a Wikipedia abstracts subset) get wired in
once the crawler exists and throughput starts to matter.

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
tests/fixtures/      small corpora and index files used only by tests
corpus/fixtures/     the working corpus
```

| | |
|---|---|
| `tokenizer` `normalize` `stopwords` `stemmer` `analyzer` | text to indexable terms |
| `corpus` `document` | reading the corpus off disk |
| `index` | the inverted index |
| `query` | query lexer and parser |
| `evaluator` | boolean and phrase evaluation |
| `ranking` | TF-IDF, BM25, top-K |
| `index_io` | the binary format and its codecs |
| `thread_pool` | workers for parallel build and scoring |

## Notes on the design

A few decisions that shape everything above.

**Positions are never renumbered.** Filters may rewrite a token or drop it, but
the survivors keep the position and byte offset they were tokenized with. Phrase
search depends on the resulting gaps being real, and byte offsets are what
snippet extraction will need to slice text back out of the source.

**Postings hold integers, not strings.** Document ids are ordinals into one
vector of names. That keeps postings small, lets intersection be a linear merge
over sorted arrays, and is what makes delta encoding possible later.

**Sorted, duplicate-free postings are a build-loop guarantee.** Documents are
added in increasing ordinal order, so deduplication is a check against the back
of the list. Three separate things rely on the resulting invariant, so the
loader re-validates it when reading an index from disk rather than trusting the
file.

**Duplicate pages are a ranking bug, not a bandwidth one.** The crawler drops a
page whose bytes it has already fetched, because indexing one document twice
inflates its terms' document frequency, drags the average document length BM25
divides by, and lets one page take two result slots.

**A query term that analyzes away carries no constraint.** Searching for
`the cat` means `cat`, not the empty set, and `NOT the` excludes nothing. Only a
query made entirely of stopwords matches nothing.

## Roadmap

- [x] Corpus ingestion and document listing
- [x] Tokenizer: case folding, punctuation, numbers, hyphens
- [x] Stopword filtering
- [x] Porter stemmer
- [x] Inverted index with term frequencies and positions
- [x] Query parsing to a boolean AST
- [x] Boolean retrieval: AND / OR / NOT over postings lists
- [x] Phrase queries via positional intersection
- [x] TF-IDF, then BM25, with top-K selection through a min-heap
- [x] Binary on-disk index with delta + variable-byte encoded postings
- [x] Parallel index construction with a deterministic merge
- [x] A crawler for building corpora from the web
- [ ] Snippet generation
- [ ] Spelling correction (edit distance over a BK-tree)
- [ ] Trie-backed autocomplete
- [ ] Incremental indexing
