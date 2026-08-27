"""`search or <corpus_dir> <word>...` -- postings union."""

NAME = "Boolean OR"
ORDER = 180

CASES = [
    {
        "name": "two words keep documents holding either",
        "argv": ["or", "tests/fixtures/tiny", "cat", "dog"],
        "stdout": "alpha\nbeta\ngamma",
    },
    {
        "name": "the union is ascending and free of duplicates",
        "argv": ["or", "corpus/fixtures", "cat", "mat"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "swapping the words gives the same set",
        "argv": ["or", "corpus/fixtures", "mat", "cat"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "a word united with itself is unchanged",
        "argv": ["or", "corpus/fixtures", "cat", "cat"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "a word with no postings adds nothing",
        "argv": ["or", "corpus/fixtures", "cat", "zzz"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "three words union together",
        "argv": ["or", "corpus/fixtures", "cat", "oven", "planet"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008\ndoc_018\ndoc_025\ndoc_026\ndoc_029",
    },
    {
        "name": "a stopword contributes nothing to the union",
        "argv": ["or", "corpus/fixtures", "cat", "the"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "an empty corpus unions to nothing",
        "argv": ["or", "tests/fixtures/empty", "cat", "dog"],
        "stdout": "",
    },
    {
        "name": "a missing corpus directory is reported",
        "argv": ["or", "tests/fixtures/nope", "cat", "dog"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "one word is not enough for a union",
        "argv": ["or", "tests/fixtures/tiny", "cat"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
