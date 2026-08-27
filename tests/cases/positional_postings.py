"""`search positions <corpus_dir> <word>` -- positional postings."""

NAME = "Positional postings"
ORDER = 140

CASES = [
    {
        "name": "a term occurring twice lists both positions, ascending",
        "argv": ["positions", "tests/fixtures/tiny", "documents"],
        "stdout": "alpha 1 5\nbeta 1 5\ngamma 1 5",
    },
    {
        "name": "positions come from the original token stream, so stopword gaps survive",
        "argv": ["positions", "tests/fixtures/tiny", "cat"],
        "stdout": "alpha 8\ngamma 8",
    },
    {
        "name": "a term adjacent to another has the neighbouring position",
        "argv": ["positions", "tests/fixtures/tiny", "tiny"],
        "stdout": "alpha 4\nbeta 4\ngamma 4",
    },
    {
        "name": "positions across the working corpus",
        "argv": ["positions", "corpus/fixtures", "cat"],
        "stdout": "doc_001 0 4 10 24\ndoc_002 15\ndoc_003 2 5 13 18 24\ndoc_004 2 6 18\ndoc_005 16\ndoc_006 18\ndoc_008 19",
    },
    {
        "name": "the query word is analyzed before lookup",
        "argv": ["positions", "tests/fixtures/tiny", "Cats"],
        "stdout": "alpha 8\ngamma 8",
    },
    {
        "name": "a word in no document reports nothing",
        "argv": ["positions", "tests/fixtures/tiny", "elephant"],
        "stdout": "",
    },
    {
        "name": "a stopword analyzes away and reports nothing",
        "argv": ["positions", "tests/fixtures/tiny", "the"],
        "stdout": "",
    },
    {
        "name": "an empty corpus reports nothing",
        "argv": ["positions", "tests/fixtures/empty", "cat"],
        "stdout": "",
    },
    {
        "name": "term frequency still matches the number of positions",
        "argv": ["tf", "tests/fixtures/tiny", "documents"],
        "stdout": "alpha 2\nbeta 2\ngamma 2",
    },
    {
        "name": "document frequency is unchanged by the new field",
        "argv": ["index-terms", "tests/fixtures/tiny"],
        "stdout": "alpha 1\nbeta 1\ncat 2\ndocument 3\ndog 2\nfirst 1\ngamma 1\nmention 3\nsecond 1\nthird 1\ntini 3",
    },
    {
        "name": "a multi-word argument is rejected",
        "argv": ["positions", "tests/fixtures/tiny", "tiny document"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing corpus directory is reported",
        "argv": ["positions", "tests/fixtures/nope", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "positions with no word is rejected",
        "argv": ["positions", "tests/fixtures/tiny"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
