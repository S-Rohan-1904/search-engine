"""`--threads <n>` on the ranking commands -- sharding by document must not change a score."""

NAME = "Parallel query evaluation"
ORDER = 330

CASES = [
    {
        "name": "single-threaded BM25",
        "argv": ["bm25", "--threads", "1", "corpus/fixtures", "cat", "mat"],
        "stdout": "doc_001 7.4906\ndoc_003 2.5076\ndoc_004 2.1294\ndoc_002 1.4405\ndoc_006 1.4405\ndoc_008 1.3676\ndoc_005 1.2710",
    },
    {
        "name": "four threads give identical scores",
        "argv": ["bm25", "--threads", "4", "corpus/fixtures", "cat", "mat"],
        "stdout": "doc_001 7.4906\ndoc_003 2.5076\ndoc_004 2.1294\ndoc_002 1.4405\ndoc_006 1.4405\ndoc_008 1.3676\ndoc_005 1.2710",
    },
    {
        "name": "so do sixteen",
        "argv": ["bm25", "--threads", "16", "corpus/fixtures", "cat", "mat"],
        "stdout": "doc_001 7.4906\ndoc_003 2.5076\ndoc_004 2.1294\ndoc_002 1.4405\ndoc_006 1.4405\ndoc_008 1.3676\ndoc_005 1.2710",
    },
    {
        "name": "single-threaded TF-IDF",
        "argv": ["tfidf", "--threads", "1", "corpus/fixtures", "cat", "mat"],
        "stdout": "doc_001 11.5890\ndoc_003 3.7975\ndoc_004 3.0541\ndoc_002 1.4553\ndoc_005 1.4553\ndoc_006 1.4553\ndoc_008 1.4553",
    },
    {
        "name": "four threads give identical scores there too",
        "argv": ["tfidf", "--threads", "4", "corpus/fixtures", "cat", "mat"],
        "stdout": "doc_001 11.5890\ndoc_003 3.7975\ndoc_004 3.0541\ndoc_002 1.4553\ndoc_005 1.4553\ndoc_006 1.4553\ndoc_008 1.4553",
    },
    {
        "name": "top-k over a sharded ranking",
        "argv": ["top", "--threads", "8", "corpus/fixtures", "3", "cat", "mat"],
        "stdout": "doc_001 11.5890\ndoc_003 3.7975\ndoc_004 3.0541",
    },
    {
        "name": "and with BM25",
        "argv": ["top", "--bm25", "--threads", "8", "corpus/fixtures", "3", "cat", "mat"],
        "stdout": "doc_001 7.4906\ndoc_003 2.5076\ndoc_004 2.1294",
    },
    {
        "name": "more threads than documents",
        "argv": ["bm25", "--threads", "16", "tests/fixtures/tiny", "cat"],
        "stdout": "alpha 0.4789\ngamma 0.4532",
    },
    {
        "name": "a saved index shards the same way",
        "argv": ["bm25", "--threads", "4", "tests/fixtures/index/good.bin", "cat"],
        "stdout": "alpha 0.4789\ngamma 0.4532",
    },
    {
        "name": "an empty corpus shards to nothing",
        "argv": ["bm25", "--threads", "8", "tests/fixtures/empty", "cat"],
        "stdout": "",
    },
    {
        "name": "a query matching nothing",
        "argv": ["bm25", "--threads", "8", "corpus/fixtures", "elephant"],
        "stdout": "",
    },
    {
        "name": "a term in every document, where every shard contributes",
        "argv": ["bm25", "--threads", "8", "tests/fixtures/tiny", "document"],
        "stdout": "alpha 0.1860\nbeta 0.1860\ngamma 0.1790",
    },
    {
        "name": "zero threads is rejected",
        "argv": ["bm25", "--threads", "0", "corpus/fixtures", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a non-numeric thread count is rejected",
        "argv": ["top", "--threads", "x", "corpus/fixtures", "3", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
]
