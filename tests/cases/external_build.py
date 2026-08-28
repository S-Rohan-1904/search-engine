"""`search index-build` -- building an index in blocks, without holding it all in memory."""

NAME = "External index construction"
ORDER = 510

CASES = [
    {
        "name": "the shape of what was built is reported",
        "argv": ["index-build", "--block", "7", "corpus/fixtures", "build/ext.idx"],
        "stdout": "documents 30\nterms 235\npostings 343\nblocks 5",
    },
    {
        "name": "a block larger than the corpus is one block",
        "argv": ["index-build", "--block", "1000", "corpus/fixtures", "build/ext-one.idx"],
        "stdout": "documents 30\nterms 235\npostings 343\nblocks 1",
    },
    {
        "name": "a block of one document is one block per document",
        "argv": ["index-build", "--block", "1", "tests/fixtures/tiny", "build/ext-tiny.idx"],
        "stdout": "documents 3\nterms 11\npostings 19\nblocks 3",
    },
    {
        # The block size decides peak memory and nothing else. If it changed
        # the output, the two builds would not be interchangeable and every
        # number measured with one would need re-measuring with the other.
        "name": "the index is what a full build produces",
        "argv": ["index-stats", "build/ext.idx"],
        "stdout": "documents: 30\nterms: 235\npostings: 343",
    },
    {
        "name": "and it answers queries the same way",
        "argv": ["top", "--bm25", "build/ext.idx", "3", "cat"],
        "stdout": "doc_003 2.5076\ndoc_001 2.3900\ndoc_004 2.1294",
    },
    {
        "name": "a corpus file works as a source too",
        "argv": ["index-build", "--block", "2", "tests/fixtures/corpusfile/tiny.corpus",
                 "build/ext-cf.idx"],
        "stdout": "documents 3\nterms 11\npostings 19\nblocks 2",
    },
    {
        "name": "a block size of zero is rejected",
        "argv": ["index-build", "--block", "0", "corpus/fixtures", "build/ext-bad.idx"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing corpus is reported",
        "argv": ["index-build", "tests/fixtures/nope", "build/ext-bad.idx"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unknown option is rejected",
        "argv": ["index-build", "--bogus", "1", "corpus/fixtures", "build/ext-bad.idx"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "index-build with no destination is rejected",
        "argv": ["index-build", "corpus/fixtures"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
