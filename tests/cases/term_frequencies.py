"""`search tf <corpus_dir> <word>` -- per-document term frequencies."""

NAME = "Term frequencies"
ORDER = 130

CASES = [
    {
        "name": "a term occurring once per document reports a count of one",
        "argv": ["tf", "tests/fixtures/tiny", "cat"],
        "stdout": "alpha 1\ngamma 1",
    },
    {
        "name": "a term in both title and body is counted twice",
        "argv": ["tf", "tests/fixtures/tiny", "documents"],
        "stdout": "alpha 2\nbeta 2\ngamma 2",
    },
    {
        "name": "counts vary across the working corpus",
        "argv": ["tf", "corpus/fixtures", "cat"],
        "stdout": "doc_001 4\ndoc_002 1\ndoc_003 5\ndoc_004 3\ndoc_005 1\ndoc_006 1\ndoc_008 1",
    },
    {
        "name": "the query word is analyzed before lookup, as with postings",
        "argv": ["tf", "tests/fixtures/tiny", "Cats"],
        "stdout": "alpha 1\ngamma 1",
    },
    {
        "name": "a word in no document reports nothing",
        "argv": ["tf", "tests/fixtures/tiny", "elephant"],
        "stdout": "",
    },
    {
        "name": "a stopword analyzes away and reports nothing",
        "argv": ["tf", "tests/fixtures/tiny", "the"],
        "stdout": "",
    },
    {
        "name": "an empty corpus reports nothing",
        "argv": ["tf", "tests/fixtures/empty", "cat"],
        "stdout": "",
    },
    {
        "name": "document frequency is unchanged by the new counts",
        "argv": ["index-terms", "tests/fixtures/tiny"],
        "stdout": "alpha 1\nbeta 1\ncat 2\ndocument 3\ndog 2\nfirst 1\ngamma 1\nmention 3\nsecond 1\nthird 1\ntini 3",
    },
    {
        "name": "a multi-word argument is rejected",
        "argv": ["tf", "tests/fixtures/tiny", "tiny document"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing corpus directory is reported",
        "argv": ["tf", "tests/fixtures/nope", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "tf with no word is rejected",
        "argv": ["tf", "tests/fixtures/tiny"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
