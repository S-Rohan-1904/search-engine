"""`search top [--bm25] <corpus_dir> <k> <word>...` -- the k best documents."""

NAME = "Top-K retrieval"
ORDER = 240

CASES = [
    {
        "name": "the top three of a longer ranking",
        "argv": ["top", "corpus/fixtures", "3", "cat"],
        "stdout": "doc_003 3.7975\ndoc_001 3.4727\ndoc_004 3.0541",
    },
    {
        "name": "the same three the full ranking starts with",
        "argv": ["tfidf", "corpus/fixtures", "cat"],
        "stdout": "doc_003 3.7975\ndoc_001 3.4727\ndoc_004 3.0541\ndoc_002 1.4553\ndoc_005 1.4553\ndoc_006 1.4553\ndoc_008 1.4553",
    },
    {
        "name": "k of one returns the single best document",
        "argv": ["top", "corpus/fixtures", "1", "cat", "mat"],
        "stdout": "doc_001 11.5890",
    },
    {
        "name": "k of zero returns nothing",
        "argv": ["top", "corpus/fixtures", "0", "cat"],
        "stdout": "",
    },
    {
        "name": "k larger than the result set returns everything",
        "argv": ["top", "corpus/fixtures", "100", "cat"],
        "stdout": "doc_003 3.7975\ndoc_001 3.4727\ndoc_004 3.0541\ndoc_002 1.4553\ndoc_005 1.4553\ndoc_006 1.4553\ndoc_008 1.4553",
    },
    {
        "name": "ties are resolved the same way the full ranking resolves them",
        "argv": ["top", "corpus/fixtures", "4", "cat"],
        "stdout": "doc_003 3.7975\ndoc_001 3.4727\ndoc_004 3.0541\ndoc_002 1.4553",
    },
    {
        "name": "several query words combine before selection",
        "argv": ["top", "corpus/fixtures", "5", "cat", "mat", "sat"],
        "stdout": "doc_001 14.9902\ndoc_003 3.7975\ndoc_004 3.0541\ndoc_002 1.4553\ndoc_005 1.4553",
    },
    {
        "name": "an unknown word leaves nothing to select from",
        "argv": ["top", "corpus/fixtures", "3", "elephant"],
        "stdout": "",
    },
    {
        "name": "an empty corpus returns nothing",
        "argv": ["top", "tests/fixtures/empty", "3", "cat"],
        "stdout": "",
    },
    {
        "name": "the bm25 flag selects the other scorer",
        "argv": ["top", "--bm25", "corpus/fixtures", "3", "cat"],
        "stdout": "doc_003 2.5076\ndoc_001 2.3900\ndoc_004 2.1294",
    },
    {
        "name": "a non-numeric k is rejected",
        "argv": ["top", "corpus/fixtures", "abc", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a negative k is rejected",
        "argv": ["top", "corpus/fixtures", "-3", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing corpus directory is reported",
        "argv": ["top", "tests/fixtures/nope", "3", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "top with no word is rejected",
        "argv": ["top", "corpus/fixtures", "3"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
