"""`search postings <corpus_dir> <word>` -- term to document ids."""

NAME = "Postings lists"
ORDER = 120

CASES = [
    {
        "name": "a term maps to the documents containing it",
        "argv": ["postings", "tests/fixtures/tiny", "cat"],
        "stdout": "alpha\ngamma",
    },
    {
        "name": "the query word goes through the same analyzer as the corpus",
        "argv": ["postings", "tests/fixtures/tiny", "Cats"],
        "stdout": "alpha\ngamma",
    },
    {
        "name": "stemming applies to the query too",
        "argv": ["postings", "tests/fixtures/tiny", "mentioning"],
        "stdout": "alpha\nbeta\ngamma",
    },
    {
        "name": "a word in every document lists them all, in id order",
        "argv": ["postings", "tests/fixtures/tiny", "documents"],
        "stdout": "alpha\nbeta\ngamma",
    },
    {
        "name": "a word in no document lists nothing",
        "argv": ["postings", "tests/fixtures/tiny", "elephant"],
        "stdout": "",
    },
    {
        "name": "a stopword analyzes away and matches nothing",
        "argv": ["postings", "tests/fixtures/tiny", "the"],
        "stdout": "",
    },
    {
        "name": "punctuation alone analyzes away",
        "argv": ["postings", "tests/fixtures/tiny", "!!!"],
        "stdout": "",
    },
    {
        "name": "postings come back in ascending document order",
        "argv": ["postings", "corpus/fixtures", "cat"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "an empty corpus has no postings for anything",
        "argv": ["postings", "tests/fixtures/empty", "cat"],
        "stdout": "",
    },
    {
        "name": "a multi-word argument is rejected",
        "argv": ["postings", "tests/fixtures/tiny", "tiny document"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing corpus directory is reported",
        "argv": ["postings", "tests/fixtures/nope", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "postings with no word is rejected",
        "argv": ["postings", "tests/fixtures/tiny"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
