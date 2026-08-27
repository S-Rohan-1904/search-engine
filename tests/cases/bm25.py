"""`search bm25 <corpus_dir> <word>...` -- ranking by BM25."""

NAME = "BM25 scoring"
ORDER = 250

CASES = [
    {
        "name": "documents containing a term are ranked by score",
        "argv": ["bm25", "corpus/fixtures", "cat"],
        "stdout": "doc_003 2.5076\ndoc_001 2.3900\ndoc_004 2.1294\ndoc_002 1.4405\ndoc_006 1.4405\ndoc_008 1.3676\ndoc_005 1.2710",
    },
    {
        "name": "length normalization reorders what TF-IDF ranked flat",
        "argv": ["tfidf", "corpus/fixtures", "cat"],
        "stdout": "doc_003 3.7975\ndoc_001 3.4727\ndoc_004 3.0541\ndoc_002 1.4553\ndoc_005 1.4553\ndoc_006 1.4553\ndoc_008 1.4553",
    },
    {
        "name": "scores from several terms add up",
        "argv": ["bm25", "corpus/fixtures", "cat", "mat"],
        "stdout": "doc_001 7.4906\ndoc_003 2.5076\ndoc_004 2.1294\ndoc_002 1.4405\ndoc_006 1.4405\ndoc_008 1.3676\ndoc_005 1.2710",
    },
    {
        "name": "word order in the query does not matter",
        "argv": ["bm25", "corpus/fixtures", "mat", "cat"],
        "stdout": "doc_001 7.4906\ndoc_003 2.5076\ndoc_004 2.1294\ndoc_002 1.4405\ndoc_006 1.4405\ndoc_008 1.3676\ndoc_005 1.2710",
    },
    {
        "name": "ties are broken by ascending document id",
        "argv": ["bm25", "tests/fixtures/tiny", "cat"],
        "stdout": "alpha 0.4789\ngamma 0.4532",
    },
    {
        "name": "a term in every document still scores above zero",
        "argv": ["bm25", "tests/fixtures/tiny", "document"],
        "stdout": "alpha 0.1860\nbeta 0.1860\ngamma 0.1790",
    },
    {
        "name": "the query word is analyzed like the corpus was",
        "argv": ["bm25", "corpus/fixtures", "CATS"],
        "stdout": "doc_003 2.5076\ndoc_001 2.3900\ndoc_004 2.1294\ndoc_002 1.4405\ndoc_006 1.4405\ndoc_008 1.3676\ndoc_005 1.2710",
    },
    {
        "name": "a stopword contributes nothing",
        "argv": ["bm25", "corpus/fixtures", "the"],
        "stdout": "",
    },
    {
        "name": "an unknown word ranks nothing",
        "argv": ["bm25", "corpus/fixtures", "elephant"],
        "stdout": "",
    },
    {
        "name": "an empty corpus ranks nothing",
        "argv": ["bm25", "tests/fixtures/empty", "cat"],
        "stdout": "",
    },
    {
        "name": "a repeated word counts twice",
        "argv": ["bm25", "tests/fixtures/tiny", "cat", "cat"],
        "stdout": "alpha 0.9578\ngamma 0.9063",
    },
    {
        "name": "top --bm25 agrees with the head of the full bm25 ranking",
        "argv": ["top", "--bm25", "corpus/fixtures", "4", "cat", "mat"],
        "stdout": "doc_001 7.4906\ndoc_003 2.5076\ndoc_004 2.1294\ndoc_002 1.4405",
    },
    {
        "name": "a missing corpus directory is reported",
        "argv": ["bm25", "tests/fixtures/nope", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "bm25 with no word is rejected",
        "argv": ["bm25", "corpus/fixtures"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
