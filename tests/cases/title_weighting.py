"""`--scorer bm25f` -- a term in the title counts for more than the same term in the body."""

NAME = "Title weighting"
ORDER = 545

BM25 = "doc_b\t0.6373\ndoc_a\t0.4606"
BM25F = "doc_a\t0.3160\ndoc_b\t0.2990"

CASES = [
    {
        "name": "plain BM25 prefers the body that repeats the word",
        "argv": ["query", "--tsv", "tests/fixtures/fields", "rust"],
        "stdout": BM25,
    },
    {
        "name": "BM25F prefers the page whose title is the word",
        "argv": ["query", "--tsv", "--scorer", "bm25f", "tests/fixtures/fields", "rust"],
        "stdout": BM25F,
    },
    {
        "name": "an index can be saved",
        "argv": ["index-write", "tests/fixtures/fields", "build/fields.idx"],
        "stdout": "",
    },
    {
        "name": "or built one document per block",
        "argv": ["index-build", "--block", "1", "tests/fixtures/fields", "build/fields-blocks.idx"],
        "stdout": "documents 3\nterms 16\npostings 18\nblocks 3",
    },
    {
        "name": "the title boundary survives a saved index",
        "argv": ["query", "--tsv", "--scorer", "bm25f", "build/fields.idx", "rust"],
        "stdout": BM25F,
    },
    {
        "name": "and an index built in blocks",
        "argv": ["query", "--tsv", "--scorer", "bm25f", "build/fields-blocks.idx", "rust"],
        "stdout": BM25F,
    },
    {
        "name": "the REPL can switch to it",
        "argv": ["repl", "--tsv", "tests/fixtures/fields"],
        "stdin": ":scorer bm25f\nrust\n:settings\n",
        "stdout": BM25F + "\nlimit 10\nscorer bm25f\nsnippet off\npagerank 0",
    },
    {
        "name": "a word in no title scores as body text alone",
        "argv": ["query", "--tsv", "--scorer", "bm25f", "tests/fixtures/fields", "appears"],
        "stdout": "doc_b\t0.6240",
    },
]
