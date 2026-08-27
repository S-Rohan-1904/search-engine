"""`search <command> <index_file>` -- every query command accepts a saved index."""

NAME = "Querying a saved index"
ORDER = 270

CASES = [
    {
        "name": "postings come back from a saved index",
        "argv": ["postings", "tests/fixtures/index/good.bin", "cat"],
        "stdout": "alpha\ngamma",
    },
    {
        "name": "and from the corpus that produced it",
        "argv": ["postings", "tests/fixtures/tiny", "cat"],
        "stdout": "alpha\ngamma",
    },
    {
        "name": "term frequencies survive the round trip",
        "argv": ["tf", "tests/fixtures/index/good.bin", "documents"],
        "stdout": "alpha 2\nbeta 2\ngamma 2",
    },
    {
        "name": "positions survive the round trip",
        "argv": ["positions", "tests/fixtures/index/good.bin", "documents"],
        "stdout": "alpha 1 5\nbeta 1 5\ngamma 1 5",
    },
    {
        "name": "document lengths survive the round trip",
        "argv": ["lengths", "tests/fixtures/index/good.bin"],
        "stdout": "alpha 7\nbeta 7\ngamma 8\naverage 7.33",
    },
    {
        "name": "boolean queries run against a saved index",
        "argv": ["match", "tests/fixtures/index/good.bin", "cat AND NOT dog"],
        "stdout": "alpha",
    },
    {
        "name": "phrase queries run against a saved index",
        "argv": ["phrase", "tests/fixtures/index/good.bin", "tiny document"],
        "stdout": "alpha\nbeta\ngamma",
    },
    {
        "name": "ranking runs against a saved index",
        "argv": ["bm25", "tests/fixtures/index/good.bin", "cat"],
        "stdout": "alpha 0.4789\ngamma 0.4532",
    },
    {
        "name": "top-k runs against a saved index",
        "argv": ["top", "tests/fixtures/index/good.bin", "2", "cat", "dog"],
        "stdout": "gamma 0.8109\nalpha 0.4055",
    },
    {
        "name": "set operations run against a saved index",
        "argv": ["and", "tests/fixtures/index/good.bin", "cat", "dog"],
        "stdout": "gamma",
    },
    {
        "name": "a file that is not an index is rejected",
        "argv": ["index-stats", "tests/fixtures/index/bad_magic.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an empty file is rejected",
        "argv": ["index-stats", "tests/fixtures/index/empty.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unknown version is rejected",
        "argv": ["index-stats", "tests/fixtures/index/bad_version.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unknown encoding byte is rejected",
        "argv": ["index-stats", "tests/fixtures/index/bad_encoding.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a truncated file is rejected",
        "argv": ["index-stats", "tests/fixtures/index/truncated.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "trailing bytes are rejected",
        "argv": ["index-stats", "tests/fixtures/index/trailing.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a plain text file is rejected",
        "argv": ["index-stats", "corpus/fixtures/doc_001.txt"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a path that is neither directory nor file is reported",
        "argv": ["index-stats", "tests/fixtures/nope"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
]
