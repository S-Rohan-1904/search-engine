"""`search crawl --out` -- a crawl produces a corpus the rest of the engine can index."""

NAME = "Fetching and writing a corpus"
ORDER = 370

CASES = [
    {
        "name": "a crawl writes its pages as a corpus",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--out", "build/crawled", "http://example.com/"],
        "stdout": "0 200 http://example.com/\n1 200 http://example.com/a.html\n1 200 http://example.com/b.html\n1 200 http://example.com/deep/c.html\n1 200 http://example.com/private/public.html",
    },
    {
        "name": "the written corpus lists its documents",
        "argv": ["docs", "build/crawled"],
        "stdout": "doc_0001\ndoc_0002\ndoc_0003\ndoc_0004\ndoc_0005",
    },
    {
        "name": "the first document keeps the page title",
        "argv": ["show", "build/crawled", "doc_0001"],
        "stdout": "id: doc_0001\ntitle: Example Home\n\nExample Home Welcome & hello The cat sat on the mat. See also page A and page B. deep page secret public manual another host mail top",
    },
    {
        "name": "the crawl indexes like any other corpus",
        "argv": ["index-stats", "build/crawled"],
        "stdout": "documents: 5\nterms: 39\npostings: 47",
    },
    {
        "name": "boolean queries run against a crawled corpus",
        "argv": ["match", "build/crawled", "cat AND mat"],
        "stdout": "doc_0001",
    },
    {
        "name": "ranking runs against it too",
        "argv": ["bm25", "build/crawled", "planet"],
        "stdout": "doc_0004 1.6052",
    },
    {
        "name": "script and style contents are not indexed",
        "argv": ["match", "build/crawled", "indexable"],
        "stdout": "",
    },
    {
        "name": "a disallowed page contributed nothing",
        "argv": ["match", "build/crawled", "nobody"],
        "stdout": "",
    },
    {
        "name": "the duplicate page was written only once",
        "argv": ["match", "build/crawled", "welcome"],
        "stdout": "doc_0001",
    },
    {
        "name": "a limited crawl writes only what it fetched",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--out", "build/crawled-small", "--max-pages", "2", "http://example.com/"],
        "stdout": "0 200 http://example.com/\n1 200 http://example.com/a.html",
    },
    {
        "name": "and the corpus is that small",
        "argv": ["index-stats", "build/crawled-small"],
        "stdout": "documents: 2\nterms: 26\npostings: 29",
    },
    {
        "name": "a bad seed is reported before anything is written",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--out", "build/crawled-bad", "ftp://x/"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
]
