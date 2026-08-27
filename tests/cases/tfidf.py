"""`search tfidf <corpus_dir> <word>...` -- ranking by TF-IDF."""

NAME = "TF-IDF scoring"
ORDER = 230

CASES = [
    {
        "name": "documents containing a term are ranked by score",
        "argv": ["tfidf", "corpus/fixtures", "cat"],
        "stdout": "doc_003 3.7975\ndoc_001 3.4727\ndoc_004 3.0541\ndoc_002 1.4553\ndoc_005 1.4553\ndoc_006 1.4553\ndoc_008 1.4553",
    },
    {
        "name": "ties are broken by ascending document id",
        "argv": ["tfidf", "tests/fixtures/tiny", "cat"],
        "stdout": "alpha 0.4055\ngamma 0.4055",
    },
    {
        "name": "scores from several terms add up",
        "argv": ["tfidf", "corpus/fixtures", "cat", "mat"],
        "stdout": "doc_001 11.5890\ndoc_003 3.7975\ndoc_004 3.0541\ndoc_002 1.4553\ndoc_005 1.4553\ndoc_006 1.4553\ndoc_008 1.4553",
    },
    {
        "name": "word order in the query does not matter",
        "argv": ["tfidf", "corpus/fixtures", "mat", "cat"],
        "stdout": "doc_001 11.5890\ndoc_003 3.7975\ndoc_004 3.0541\ndoc_002 1.4553\ndoc_005 1.4553\ndoc_006 1.4553\ndoc_008 1.4553",
    },
    {
        "name": "a repeated word counts twice",
        "argv": ["tfidf", "tests/fixtures/tiny", "cat", "cat"],
        "stdout": "alpha 0.8109\ngamma 0.8109",
    },
    {
        "name": "a term in every document scores zero for all of them",
        "argv": ["tfidf", "tests/fixtures/tiny", "document"],
        "stdout": "alpha 0.0000\nbeta 0.0000\ngamma 0.0000",
    },
    {
        "name": "the query word is analyzed like the corpus was",
        "argv": ["tfidf", "corpus/fixtures", "CATS"],
        "stdout": "doc_003 3.7975\ndoc_001 3.4727\ndoc_004 3.0541\ndoc_002 1.4553\ndoc_005 1.4553\ndoc_006 1.4553\ndoc_008 1.4553",
    },
    {
        "name": "a stopword contributes nothing",
        "argv": ["tfidf", "corpus/fixtures", "the"],
        "stdout": "",
    },
    {
        "name": "an unknown word ranks nothing",
        "argv": ["tfidf", "corpus/fixtures", "elephant"],
        "stdout": "",
    },
    {
        "name": "a known word beside an unknown one still ranks",
        "argv": ["tfidf", "tests/fixtures/tiny", "cat", "elephant"],
        "stdout": "alpha 0.4055\ngamma 0.4055",
    },
    {
        "name": "an empty corpus ranks nothing",
        "argv": ["tfidf", "tests/fixtures/empty", "cat"],
        "stdout": "",
    },
    {
        "name": "a rare term outranks a common one",
        "argv": ["tfidf", "corpus/fixtures", "merge", "sort"],
        "stdout": "doc_011 13.8750",
    },
    {
        "name": "a missing corpus directory is reported",
        "argv": ["tfidf", "tests/fixtures/nope", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "tfidf with no word is rejected",
        "argv": ["tfidf", "corpus/fixtures"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
