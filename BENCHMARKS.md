# Benchmarks

Every figure here comes from a command in this repository, and the command is
printed beside it. A number without a way to reproduce it is not a measurement.

**Machine:** Apple M2 (4 performance + 4 efficiency cores), macOS, gcc 15.1,
`-O2`, `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`, no
warnings.

**Corpus:** Simple English Wikipedia, CirrusSearch dump of 2025-12-29, imported
with `search wiki-import`. Measured 2026-11-10, after the analyzer began
splitting compounds at hyphens and slashes, which is why there are fewer
distinct terms and more postings than an earlier run of the same corpus
(1,443,435 and 25,643,127).

    documents        278,214
    distinct terms 1,204,562
    postings      26,162,711
    corpus bytes 424,874,389

Query timings are warm: the page cache holds the index. Cold runs are several
times slower and are not what an interactive tool does after its first query.

Everything in the four sections below comes from one run of
`./tools/bench.sh corpus/wikipedia/simplewiki.corpus`, except the latency rows,
which were re-measured with `search bench query` after the change described
under them.

---

## Index construction

| Measurement      | Value                    | Reproduce with                            |
| ---------------- | ------------------------ | ----------------------------------------- |
| Build, 1 thread  | 28.8 s                   | `search bench build --threads 1 <corpus>` |
| Build, 8 threads | 8.7 s                    | `search bench build --threads 8 <corpus>` |
| Parallel speedup | 3.30x                    | the two rows above                        |
| Ingest rate      | 31,883 docs/s, 46.4 MB/s | `search bench build --threads 8 <corpus>` |
| Corpus open      | 25 ms                    | `open_ms` in the same output              |

`bench build` reports the build in three phases, `open_ms`, `index_ms` and
`merge_ms`, because a single total cannot say where a speedup stops. At 8
threads the serial merge is 2.0 s of the 8.7 s, which is what caps the speedup
at 3.30x rather than anything about the parallel phase.

## Index size

| Measurement           | Value               | Reproduce with              |
| --------------------- | ------------------- | --------------------------- |
| Uncompressed          | 1,041,969,371 bytes | `search index-size <index>` |
| Delta + variable-byte | 186,289,329 bytes   | same                        |
| Compression ratio     | 5.59x               | same                        |

## Query latency

25 queries (keyword, boolean and phrase), 5 repeats each.

| Measurement                            | Value         | Reproduce with                                     |
| -------------------------------------- | ------------- | -------------------------------------------------- |
| Startup, one query, total process time | 35 ms         | `time search query <index> 'solar system'`         |
| p50                                    | 1.35 ms       | `search bench query --repeats 5 <index> <queries>` |
| p95                                    | 12.0 ms       | same                                               |
| p99                                    | 17.1 ms       | same                                               |
| Throughput                             | 327 queries/s | same                                               |
| p50, with snippets                     | 2.64 ms       | add `--snippet`                                    |
| p95, with snippets                     | 14.0 ms       | same                                               |
| Peak memory, serving                   | 97 MB         | `/usr/bin/time -l` on the above                    |

### Where query time went

Sampling the query benchmark with macOS `sample` put about 37% of the time in
`std::sort` over every scored document, inside the scorer, and most of the
rest in hash-map bookkeeping around it. BM25 arithmetic itself was a few
percent. The full sort was wasted: the query path re-ranks with a bounded heap
of the top k anyway. The query path now asks for scores unordered. Same 250
results for the 25 queries; measured on the same index, minutes apart:

| Measurement | Sorted     | Unordered  |
| ----------- | ---------- | ---------- |
| p50         | 2.31 ms    | 1.35 ms    |
| p95         | 19.2 ms    | 12.0 ms    |
| Throughput  | 202 q/s    | 327 q/s    |

The same profile is why there is no WAND or block-max pruning here. Those
avoid scoring documents that cannot reach the top k; scoring was not the cost.

## Memory during construction

| Build                       | Blocks | Peak memory |
| --------------------------- | ------ | ----------- |
| In memory, 8 threads        | 1      | 3.12 GB     |
| `index-build --block 27822` | 10     | 1.06 GB     |

The block build produces a file byte-identical to the in-memory build's
(checked with `cmp` by the same script), so the block size is a memory setting
and not a correctness question.

## Relevance

Measured against BEIR, whose corpora, queries and relevance judgements are
public and independent of this code, and whose BM25 baselines are published per
dataset.

| Dataset    | Documents | Queries | BM25   | BM25F  | Published BEIR BM25 |
| ---------- | --------- | ------- | ------ | ------ | ------------------- |
| SciFact    | 5,183     | 300     | 0.6845 | 0.6925 | 0.665               |
| TREC-COVID | 171,331   | 50      | 0.6632 | 0.6853 | 0.656               |

All figures are nDCG@10.

    search evaluate [--scorer bm25f] <index> <queries.jsonl> <qrels.tsv>

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
further +0.073 in the reference, and is what the BM25F scorer below
addresses. Anserini's k1=0.9 and b=0.4 lower the score in both.

### BM25F: weighting the title

BM25 over one stream counts a word in a title the same as the same word deep in
an abstract. `--scorer bm25f` keeps both: each document records where its title
ends in the token stream and how many indexed terms the title holds, so a
term's frequency splits by position into title and body. Each part is
normalized by its own field's length and average, the title part is weighted
2x, and the sum is saturated once, as BM25F does:

    w(d) = 2 * tf_title / (1 - b + b * len_title / avg_title)
         +     tf_body  / (1 - b + b * len_body  / avg_body)
    score(d) = sum over terms of idf * w(d) / (k1 + w(d))

The weight was chosen with the Python reference before the engine had a line of
BM25F in it, sweeping 1, 2, 3 and 5:

| Title weight | TREC-COVID | SciFact |
| ------------ | ---------- | ------- |
| 1            | 0.6640     | 0.6852  |
| 2            | 0.6852     | 0.6899  |
| 3            | 0.6830     | 0.6938  |
| 5            | 0.6859     | 0.6924  |

2 and 3 are within noise of each other; 2 was taken. The engine then measured
0.6853 and 0.6925, agreeing with the reference to within its analyzer
differences. The weight was picked on the same test queries it is reported on,
which is a mild optimism; the gain is in both datasets and at every weight
above 1, which is the part that does not depend on the pick.

Storing the title boundary added one 8-byte table per document to the index
file (format version 4): 2.2 MB on Simple English Wikipedia, about 1.2%.

## PageRank: helps find home pages, hurts everything else

`--pagerank <w>` multiplies the text score by `1 + w * ln(1 + N * pagerank)`.
N times a page's PageRank is its rank relative to the average page, so a
weight means the same thing at any graph size, and the factor never drops
below 1, so link authority reorders matching pages but cannot pull in a page
that does not match.

### Within sites

**Corpus.** A real crawl of three documentation sites, each with robots.txt
honoured and a one-second delay between requests:

| Site                         | Prefix           | Pages | Links   |
| ---------------------------- | ---------------- | ----- | ------- |
| docs.python.org              | `/3/`            | 595   | 110,960 |
| www.postgresql.org           | `/docs/current/` | 1,148 | 23,245  |
| docs.djangoproject.com       | `/en/6.1/`       | 681   | 27,981  |
| Total, merged into one index |                  | 2,424 | 162,186 |

    search crawl --delay 1000 --max-pages 2000 --max-depth 12 --prefix /3/ \
        --out corpus/crawl/pydocs https://docs.python.org/3/
    ./tools/merge_crawls.py corpus/crawl/docs corpus/crawl/pydocs ...
    search index-write corpus/crawl/docs docs.idx

The Python crawl stopped because its frontier ran out: 595 pages is all of
`/3/` that links reach from the root.

**Measurement.** A crawl has no relevance judgements, so the test is known-item
search with targets chosen mechanically rather than by hand: each page whose
title is unique gives one query, the title without the site's boilerplate
(the json module's page gives "json JSON encoder and decoder", PostgreSQL's gives "CREATE TABLE"),
and the right answer is that page. 2,338 queries.

| Scorer | PageRank weight | MRR@10 | Success@1 |
| ------ | --------------- | ------ | --------- |
| BM25   | 0               | 0.8740 | 0.8058    |
| BM25   | 0.05            | 0.8299 | 0.7374    |
| BM25   | 0.1             | 0.7855 | 0.6779    |
| BM25   | 0.25            | 0.6598 | 0.5081    |
| BM25   | 0.5             | 0.4326 | 0.1527    |
| BM25F  | 0               | 0.8879 | 0.8251    |
| BM25F  | 0.1             | 0.7987 | 0.6963    |

**Every weight is worse than none.** The highest PageRank pages in the Python
crawl are the module index, the general index, the copyright page and the bug
report page: the pages in every page's navigation bar. Within one site,
PageRank measures the template, not the content. Removing the links that more
than 20% of pages share was tried on the Python crawl; the best result was a
tie with plain BM25 (MRR@10 0.8667 against 0.8670), never a gain.

That result is about one kind of graph. Three documentation trees that link
only to themselves have no links from one author vouching for another, which is
what PageRank was built to read. So the same measurement was repeated on a
crawl across independent sites.

### Across sites

**Corpus.** A crawl outward from the Hacker News front page, following links to
any site, at most 15 pages per site and 3 links deep, robots.txt honoured and
one second between requests to any one site:

    search crawl --all-hosts --concurrency 16 --max-per-host 15 --max-depth 3 \
        --max-pages 4000 --delay 1000 --out corpus/crawl/web https://news.ycombinator.com/

| Measurement                 | Value  |
| --------------------------- | ------ |
| Pages saved                 | 2,994  |
| Sites                       | 1,096  |
| Distinct links              | 27,942 |
| Links between two sites     | 14,713 |

The highest PageRank pages are now other sites' home pages: about.google,
atproto.com, tiktok.com, joinmastodon.org, postgresql.org, wordpress.org. That
is the graph PageRank was designed for.

**Homepage finding.** The task PageRank is known to help: the query is a site's
name, taken mechanically from its home page's title up to the first separator
("PostgreSQL" from "PostgreSQL: The world's most advanced open source
database"), and the right answer is that home page rather than any of the
site's other pages or the pages elsewhere that mention it. 148 sites had a home
page, at least one other page, and a unique name.

| Scorer | PageRank weight | MRR@10 | Success@1 |
| ------ | --------------- | ------ | --------- |
| BM25   | 0               | 0.5645 | 0.4257    |
| BM25   | 0.05            | 0.6382 | 0.5203    |
| BM25   | 0.1             | 0.6297 | 0.5068    |
| BM25   | 0.25            | 0.6150 | 0.4797    |
| BM25   | 1               | 0.5724 | 0.4527    |
| BM25F  | 0               | 0.6397 | 0.5203    |
| BM25F  | 0.1             | 0.6722 | 0.5405    |
| BM25F  | 0.25            | 0.6747 | 0.5473    |

148 queries is few, so each difference was checked per query, with a 95%
interval from 10,000 bootstrap resamples of the per-query change in reciprocal
rank:

| Comparison             | Mean change | 95% interval     | Better / worse |
| ---------------------- | ----------- | ---------------- | -------------- |
| BM25, weight 0.05      | +0.074      | +0.035 to +0.114 | 33 / 10        |
| BM25, weight 0.1       | +0.065      | +0.024 to +0.107 | 37 / 14        |
| BM25F, weight 0.1      | +0.032      | -0.011 to +0.075 | 32 / 18        |
| BM25F, weight 0.25     | +0.035      | -0.019 to +0.088 | 37 / 23        |

On BM25 the gain is real. On BM25F it is smaller and not distinguishable from
noise at this sample size: a home page usually carries the site's name in its
title, and title weighting already rewards that.

**Title lookup.** The earlier test, on this crawl: every page with a unique
title, searched for by that title, 2,387 queries.

| Scorer | PageRank weight | MRR@10 | Success@1 |
| ------ | --------------- | ------ | --------- |
| BM25   | 0               | 0.8606 | 0.8073    |
| BM25   | 0.05            | 0.8477 | 0.7834    |
| BM25   | 0.1             | 0.8236 | 0.7461    |
| BM25   | 0.25            | 0.7620 | 0.6548    |
| BM25F  | 0               | 0.8970 | 0.8517    |
| BM25F  | 0.1             | 0.8513 | 0.7763    |

Favouring well-linked pages costs accuracy when the page being looked for is an
ordinary one, across sites as within them.

### What follows

PageRank is a prior on which page a searcher wants when the text cannot decide.
For navigational queries across independent sites it is right more often than
not: +0.074 MRR@10 on plain BM25, with an interval clear of zero. For finding a
particular page, and anywhere inside one site, it is wrong more often than not.
The default weight stays 0, `--pagerank 0.05` is the setting for navigational
search over a crawl of many sites, and BM25F's title weighting, which helps on
every test here, is the stronger single change.

## Skip pointers: measured and rejected

Galloping search in postings intersection, the expected optimisation, made
intersection-heavy queries **twice as slow** (96.3 → 52.6 queries/s). The scan
was never the cost; decoding the postings was.
