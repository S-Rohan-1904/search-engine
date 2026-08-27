"""`search index-write` -- saving an index to a binary file."""

NAME = "Index persistence"
ORDER = 260

CASES = [
    {
        "name": "writing an index produces no output",
        "argv": ["index-write", "--encoding", "plain", "tests/fixtures/tiny", "build/test-index.bin"],
        "stdout": "",
    },
    {
        "name": "the saved index reports the same counts as the corpus",
        "argv": ["index-stats", "build/test-index.bin"],
        "stdout": "documents: 3\nterms: 11\npostings: 19",
    },
    {
        "name": "the corpus it came from reports those counts too",
        "argv": ["index-stats", "tests/fixtures/tiny"],
        "stdout": "documents: 3\nterms: 11\npostings: 19",
    },
    {
        "name": "the saved index holds the same dictionary",
        "argv": ["index-terms", "build/test-index.bin"],
        "stdout": "alpha 1\nbeta 1\ncat 2\ndocument 3\ndog 2\nfirst 1\ngamma 1\nmention 3\nsecond 1\nthird 1\ntini 3",
    },
    {
        "name": "delta encoding round trips",
        "argv": ["index-write", "--encoding", "delta", "tests/fixtures/tiny", "build/test-index.bin"],
        "stdout": "",
    },
    {
        "name": "a delta-encoded index reads back identically",
        "argv": ["index-terms", "build/test-index.bin"],
        "stdout": "alpha 1\nbeta 1\ncat 2\ndocument 3\ndog 2\nfirst 1\ngamma 1\nmention 3\nsecond 1\nthird 1\ntini 3",
    },
    {
        "name": "varbyte encoding round trips",
        "argv": ["index-write", "--encoding", "varbyte", "tests/fixtures/tiny", "build/test-index.bin"],
        "stdout": "",
    },
    {
        "name": "a varbyte-encoded index reads back identically",
        "argv": ["index-terms", "build/test-index.bin"],
        "stdout": "alpha 1\nbeta 1\ncat 2\ndocument 3\ndog 2\nfirst 1\ngamma 1\nmention 3\nsecond 1\nthird 1\ntini 3",
    },
    {
        "name": "varbyte is the default encoding",
        "argv": ["index-write", "tests/fixtures/tiny", "build/test-index.bin"],
        "stdout": "",
    },
    {
        "name": "the default reads back identically too",
        "argv": ["index-terms", "build/test-index.bin"],
        "stdout": "alpha 1\nbeta 1\ncat 2\ndocument 3\ndog 2\nfirst 1\ngamma 1\nmention 3\nsecond 1\nthird 1\ntini 3",
    },
    {
        "name": "an empty corpus saves and loads",
        "argv": ["index-write", "tests/fixtures/empty", "build/test-index.bin"],
        "stdout": "",
    },
    {
        "name": "an empty saved index is still an index",
        "argv": ["index-stats", "build/test-index.bin"],
        "stdout": "documents: 0\nterms: 0\npostings: 0",
    },
    {
        "name": "an unknown encoding is rejected",
        "argv": ["index-write", "--encoding", "zip", "tests/fixtures/tiny", "build/test-index.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing source is reported",
        "argv": ["index-write", "tests/fixtures/nope", "build/test-index.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unwritable destination is reported",
        "argv": ["index-write", "tests/fixtures/tiny", "/nope/x.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "index-write with no destination is rejected",
        "argv": ["index-write", "tests/fixtures/tiny"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
