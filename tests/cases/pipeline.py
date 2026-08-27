"""`search analyze` and `search analyze-doc` -- the full analysis chain."""

NAME = "The analysis pipeline"
ORDER = 100

CASES = [
    {
        "name": "stopwords and stems both apply to ordinary prose",
        "argv": ["analyze", "The cats were running quickly"],
        "stdout": "1 4 cat\n3 14 run\n4 22 quickli",
    },
    {
        "name": "a stem is taken after the stopword filter, not before",
        "argv": ["analyze", "having a lovely time"],
        "stdout": "2 9 love\n3 16 time",
    },
    {
        "name": "case is folded before anything else looks at the word",
        "argv": ["analyze", "RUNNING Cats CONNECTED"],
        "stdout": "0 0 run\n1 8 cat\n2 13 connect",
    },
    {
        "name": "dropped stopwords leave gaps in the position sequence",
        "argv": ["analyze", "the cat and the dog and the bird"],
        "stdout": "1 4 cat\n4 16 dog\n7 28 bird",
    },
    {
        "name": "a hyphenated compound survives normalization and stems as one term",
        "argv": ["analyze", "well-lit laboratories"],
        "stdout": "0 0 well-lit\n1 9 laboratori",
    },
    {
        "name": "punctuation and possessives are stripped before stemming",
        "argv": ["analyze", "Dr. O'Brien's studies"],
        "stdout": "0 0 dr\n1 4 obrien\n2 14 studi",
    },
    {
        "name": "a number is kept verbatim -- the stemmer leaves it alone",
        "argv": ["analyze", "2024 releases"],
        "stdout": "0 0 2024\n1 5 releas",
    },
    {
        "name": "text made entirely of stopwords produces no terms at all",
        "argv": ["analyze", "the and of to it is"],
        "stdout": "",
    },
    {
        "name": "empty input produces no terms",
        "argv": ["analyze", ""],
        "stdout": "",
    },
    {
        "name": "offsets are byte indices into the original text, not the term stream",
        "argv": ["analyze", "a cat, a mat"],
        "stdout": "1 2 cat\n3 9 mat",
    },
    {
        "name": "a document is analyzed as its title followed by its body",
        "argv": ["analyze-doc", "tests/fixtures/docs", "multi_para"],
        "stdout": "0 0 two\n1 4 paragraph\n3 20 first\n4 26 paragraph\n5 36 talk\n8 52 index\n10 64 second\n11 71 paragraph\n12 81 talk\n15 97 queri",
    },
    {
        "name": "body offsets are shifted past the title and the blank line",
        "argv": ["analyze-doc", "corpus/fixtures", "doc_001"],
        "stdout": "0 0 cat\n2 9 mat\n4 19 cat\n5 23 sat\n8 34 mat\n10 43 cat\n12 51 happi\n15 65 mat\n17 72 mat\n20 81 flat\n21 86 rug\n24 98 cat\n25 102 like\n27 111 sit\n30 121 everi\n31 127 afternoon",
    },
    {
        "name": "a document with an empty body still yields its title terms",
        "argv": ["analyze-doc", "tests/fixtures/docs", "empty_body"],
        "stdout": "0 0 noth",
    },
    {
        "name": "a missing corpus directory is reported",
        "argv": ["analyze-doc", "tests/fixtures/nope", "doc_001"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing document is reported",
        "argv": ["analyze-doc", "corpus/fixtures", "doc_999"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a malformed document is reported",
        "argv": ["analyze-doc", "tests/fixtures/docs", "no_title"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "analyze with no argument is rejected",
        "argv": ["analyze"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
