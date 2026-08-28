"""`search index-update` -- re-analyzing only the documents that changed."""

NAME = "Incremental indexing"
ORDER = 440

CASES = [
    {
        "name": "start from a known empty index",
        "argv": ["index-write", "tests/fixtures/empty", "build/inc-test.bin"],
        "stdout": "",
    },
    {
        "name": "the first update adds every document",
        "argv": ["index-update", "tests/fixtures/inc-a", "build/inc-test.bin"],
        "stdout": "added 3\nupdated 0\nremoved 0\nunchanged 0",
    },
    {
        "name": "running it again changes nothing",
        "argv": ["index-update", "tests/fixtures/inc-a", "build/inc-test.bin"],
        "stdout": "added 0\nupdated 0\nremoved 0\nunchanged 3",
    },
    {
        "name": "the index holds what it should",
        "argv": ["index-stats", "build/inc-test.bin"],
        "stdout": "documents: 3\nterms: 11\npostings: 11",
    },
    {
        "name": "a changed corpus reports what moved",
        "argv": ["index-update", "tests/fixtures/inc-b", "build/inc-test.bin"],
        "stdout": "added 1\nupdated 1\nremoved 1\nunchanged 1",
    },
    {
        "name": "the removed document is gone",
        "argv": ["match", "build/inc-test.bin", "bird"],
        "stdout": "",
    },
    {
        "name": "the changed document has its new terms",
        "argv": ["match", "build/inc-test.bin", "river"],
        "stdout": "doc_b\ndoc_d",
    },
    {
        "name": "and lost its old ones",
        "argv": ["match", "build/inc-test.bin", "park"],
        "stdout": "",
    },
    {
        "name": "the unchanged document still matches",
        "argv": ["match", "build/inc-test.bin", "purred"],
        "stdout": "doc_a",
    },
    {
        "name": "the new document is indexed",
        "argv": ["match", "build/inc-test.bin", "fish"],
        "stdout": "doc_d",
    },
    {
        "name": "the result is what a full build produces",
        "argv": ["index-stats", "build/inc-test.bin"],
        "stdout": "documents: 3\nterms: 10\npostings: 12",
    },
    {
        "name": "and a full build agrees",
        "argv": ["index-stats", "tests/fixtures/inc-b"],
        "stdout": "documents: 3\nterms: 10\npostings: 12",
    },
    {
        "name": "document lengths match a full build too",
        "argv": ["lengths", "build/inc-test.bin"],
        "stdout": "doc_a 5\ndoc_b 6\ndoc_d 4\naverage 5.00",
    },
    {
        "name": "as do the lengths from the corpus",
        "argv": ["lengths", "tests/fixtures/inc-b"],
        "stdout": "doc_a 5\ndoc_b 6\ndoc_d 4\naverage 5.00",
    },
    {
        "name": "updating against the same corpus is now a no-op",
        "argv": ["index-update", "tests/fixtures/inc-b", "build/inc-test.bin"],
        "stdout": "added 0\nupdated 0\nremoved 0\nunchanged 3",
    },
    {
        "name": "a missing corpus is reported",
        "argv": ["index-update", "tests/fixtures/nope", "build/inc-test.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a corrupt index is reported rather than overwritten",
        "argv": ["index-update", "tests/fixtures/inc-a", "tests/fixtures/index/truncated.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "index-update with no index file is rejected",
        "argv": ["index-update", "tests/fixtures/inc-a"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
