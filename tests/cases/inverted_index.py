"""`search index-stats` and `search index-terms` -- the inverted index."""

NAME = "Inverted index"
ORDER = 110

CASES = [
    {
        "name": "counts documents, distinct terms and postings",
        "argv": ["index-stats", "tests/fixtures/tiny"],
        "stdout": "documents: 3\nterms: 11\npostings: 19",
    },
    {
        "name": "every term with its document frequency, sorted",
        "argv": ["index-terms", "tests/fixtures/tiny"],
        "stdout": "alpha 1\nbeta 1\ncat 2\ndocument 3\ndog 2\nfirst 1\ngamma 1\nmention 3\nsecond 1\nthird 1\ntini 3",
    },
    {
        "name": "a term repeated inside one document still counts once",
        "argv": ["index-terms", "tests/fixtures/docs"],
        "stdout": "31 1\naspect 1\nbodi 4\ncolon 1\nfirst 1\nimmedi 1\nindex 1\nlot 1\nnoth 1\npad 1\nparagraph 1\nqueri 1\nratio 1\nsecond 1\nspace 1\nstart 1\ntalk 1\ntight 1\ntitl 1\ntwo 1",
    },
    {
        "name": "malformed documents are skipped rather than indexed",
        "argv": ["index-stats", "tests/fixtures/docs"],
        "stdout": "documents: 7\nterms: 20\npostings: 23",
    },
    {
        "name": "an empty corpus indexes nothing",
        "argv": ["index-stats", "tests/fixtures/empty"],
        "stdout": "documents: 0\nterms: 0\npostings: 0",
    },
    {
        "name": "an empty corpus has no terms to list",
        "argv": ["index-terms", "tests/fixtures/empty"],
        "stdout": "",
    },
    {
        "name": "the working corpus builds",
        "argv": ["index-stats", "corpus/fixtures"],
        "stdout": "documents: 30\nterms: 235\npostings: 343",
    },
    {
        "name": "documents are added in sorted id order",
        "argv": ["index-stats", "tests/fixtures/unsorted"],
        "stdout": "documents: 4\nterms: 7\npostings: 16",
    },
    {
        "name": "a missing corpus directory is reported by index-stats",
        "argv": ["index-stats", "tests/fixtures/nope"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing corpus directory is reported by index-terms",
        "argv": ["index-terms", "tests/fixtures/nope"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "index-stats with no argument is rejected",
        "argv": ["index-stats"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
