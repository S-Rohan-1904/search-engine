"""`search and <corpus_dir> <word>...` -- postings intersection."""

NAME = "Boolean AND"
ORDER = 170

CASES = [
    {
        "name": "two words keep only the documents holding both",
        "argv": ["and", "tests/fixtures/tiny", "cat", "dog"],
        "stdout": "gamma",
    },
    {
        "name": "the result is ascending, whatever order the words came in",
        "argv": ["and", "corpus/fixtures", "cat", "mat"],
        "stdout": "doc_001",
    },
    {
        "name": "swapping the words gives the same set",
        "argv": ["and", "corpus/fixtures", "mat", "cat"],
        "stdout": "doc_001",
    },
    {
        "name": "a word intersected with itself is unchanged",
        "argv": ["and", "corpus/fixtures", "cat", "cat"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "words sharing no document give nothing",
        "argv": ["and", "corpus/fixtures", "cat", "zzz"],
        "stdout": "",
    },
    {
        "name": "three words intersect down further",
        "argv": ["and", "corpus/fixtures", "cat", "mat", "sat"],
        "stdout": "doc_001",
    },
    {
        "name": "a stopword has no postings, so the intersection empties",
        "argv": ["and", "corpus/fixtures", "cat", "the"],
        "stdout": "",
    },
    {
        "name": "an unknown word empties the intersection",
        "argv": ["and", "corpus/fixtures", "cat", "elephant"],
        "stdout": "",
    },
    {
        "name": "an empty corpus intersects to nothing",
        "argv": ["and", "tests/fixtures/empty", "cat", "dog"],
        "stdout": "",
    },
    {
        "name": "a missing corpus directory is reported",
        "argv": ["and", "tests/fixtures/nope", "cat", "dog"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a multi-word argument is rejected",
        "argv": ["and", "tests/fixtures/tiny", "cat", "tiny document"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "one word is not enough for an intersection",
        "argv": ["and", "tests/fixtures/tiny", "cat"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
