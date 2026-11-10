"""`search query --pagerank` -- link authority as a multiplier on the text score."""

NAME = "Link authority in ranking"
ORDER = 540

PLAIN = "doc_0004\t0.4881\ndoc_0003\t0.4271\ndoc_0002\t0.3884\ndoc_0001\t0.3800"
BOOSTED = "doc_0004\t1.0344\ndoc_0002\t0.8232\ndoc_0003\t0.5422\ndoc_0001\t0.4664"

CASES = [
    {
        "name": "without the flag the ranking is plain BM25",
        "argv": ["query", "--tsv", "build/crawled", "page"],
        "stdout": PLAIN,
    },
    {
        "name": "a weight of zero is the same as no flag",
        "argv": ["query", "--tsv", "--pagerank", "0", "build/crawled", "page"],
        "stdout": PLAIN,
    },
    {
        "name": "a crawl directory's links.tsv lifts well-linked pages",
        "argv": ["query", "--tsv", "--pagerank", "1", "build/crawled", "page"],
        "stdout": BOOSTED,
    },
    {
        "name": "index-write keeps the PageRank beside the index",
        "argv": ["index-write", "build/crawled", "build/crawl-prior.idx"],
        "stdout": "",
    },
    {
        "name": "and the index ranks exactly as the directory did",
        "argv": ["query", "--tsv", "--pagerank", "1", "build/crawl-prior.idx", "page"],
        "stdout": BOOSTED,
    },
    {
        "name": "pagerank --out writes the file index-write keeps beside an index",
        "argv": ["pagerank", "--out", "build/crawl-prior.idx.pagerank", "build/crawled/links.tsv"],
        "stdout": "",
    },
    {
        "name": "and an index ranks with it exactly as before",
        "argv": ["query", "--tsv", "--pagerank", "1", "build/crawl-prior.idx", "page"],
        "stdout": BOOSTED,
    },
    {
        "name": "a document that does not match is not carried in by its links",
        "argv": ["query", "--tsv", "--pagerank", "5", "build/crawled", "planet"],
        "stdout": "doc_0004\t10.6443",
    },
    {
        "name": "the REPL turns it on and off",
        "argv": ["repl", "--tsv", "build/crawled"],
        "stdin": ":pagerank 1\npage\n:pagerank 0\npage\n:settings\n",
        "stdout": BOOSTED + "\n" + PLAIN + "\nlimit 10\nscorer bm25\nsnippet off\npagerank 0\npagerank-graph page\npagerank-max-words 0",
    },
    {
        "name": "a corpus without links writes no PageRank file",
        "argv": ["index-write", "corpus/fixtures", "build/fx-no-links.idx"],
        "stdout": "",
    },
    {
        "name": "an index with no PageRank beside it is an error, not a silent no-op",
        "argv": ["query", "--pagerank", "1", "build/fx-no-links.idx", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a negative weight is rejected",
        "argv": ["query", "--pagerank", "-1", "build/crawled", "page"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "so is one that is not a number",
        "argv": ["query", "--pagerank", "lots", "build/crawled", "page"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
]
