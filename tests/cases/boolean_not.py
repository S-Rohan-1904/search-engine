"""`search andnot <corpus_dir> <word> <word>` -- postings difference."""

NAME = "Boolean NOT"
ORDER = 190

CASES = [
    {
        "name": "the second word's documents are removed from the first's",
        "argv": ["andnot", "tests/fixtures/tiny", "cat", "dog"],
        "stdout": "alpha",
    },
    {
        "name": "difference is not symmetric",
        "argv": ["andnot", "tests/fixtures/tiny", "dog", "cat"],
        "stdout": "beta",
    },
    {
        "name": "subtracting a word from itself empties the list",
        "argv": ["andnot", "corpus/fixtures", "cat", "cat"],
        "stdout": "",
    },
    {
        "name": "subtracting a word with no postings changes nothing",
        "argv": ["andnot", "corpus/fixtures", "cat", "zzz"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "subtracting a stopword changes nothing",
        "argv": ["andnot", "corpus/fixtures", "cat", "the"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "subtracting from a word with no postings gives nothing",
        "argv": ["andnot", "corpus/fixtures", "zzz", "cat"],
        "stdout": "",
    },
    {
        "name": "a larger difference over the working corpus",
        "argv": ["andnot", "corpus/fixtures", "cat", "mat"],
        "stdout": "doc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "an empty corpus subtracts to nothing",
        "argv": ["andnot", "tests/fixtures/empty", "cat", "dog"],
        "stdout": "",
    },
    {
        "name": "a missing corpus directory is reported",
        "argv": ["andnot", "tests/fixtures/nope", "cat", "dog"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "difference takes exactly two words",
        "argv": ["andnot", "tests/fixtures/tiny", "cat", "dog", "bird"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
    {
        "name": "one word is not enough for a difference",
        "argv": ["andnot", "tests/fixtures/tiny", "cat"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
