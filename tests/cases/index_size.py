"""`search index-size <source>` -- what the encodings cost."""

NAME = "Index size report"
ORDER = 300

CASES = [
    {
        "name": "the working corpus, broken down by section",
        "argv": ["index-size", "corpus/fixtures"],
        "stdout": "plain 19738 header 65 documents 938 dictionary 4887 postings 8232 positions 3736\ndelta 19738 header 65 documents 938 dictionary 4887 postings 8232 positions 3736\nvarbyte 5976 header 65 documents 938 dictionary 1597 postings 1029 positions 467\nratio 3.30",
    },
    {
        "name": "the tiny fixture",
        "argv": ["index-size", "tests/fixtures/tiny"],
        "stdout": "plain 1110 header 65 documents 94 dictionary 231 postings 456 positions 176\ndelta 1110 header 65 documents 94 dictionary 231 postings 456 positions 176\nvarbyte 403 header 65 documents 94 dictionary 77 postings 57 positions 22\nratio 2.75",
    },
    {
        "name": "an empty corpus is almost all header",
        "argv": ["index-size", "tests/fixtures/empty"],
        "stdout": "plain 73 header 65 documents 8 dictionary 0 postings 0 positions 0\ndelta 73 header 65 documents 8 dictionary 0 postings 0 positions 0\nvarbyte 73 header 65 documents 8 dictionary 0 postings 0 positions 0\nratio 1.00",
    },
    {
        "name": "a saved index measures the same as the corpus it came from",
        "argv": ["index-size", "tests/fixtures/index/good.bin"],
        "stdout": "plain 1110 header 65 documents 94 dictionary 231 postings 456 positions 176\ndelta 1110 header 65 documents 94 dictionary 231 postings 456 positions 176\nvarbyte 403 header 65 documents 94 dictionary 77 postings 57 positions 22\nratio 2.75",
    },
    {
        "name": "and so does the corpus itself",
        "argv": ["index-size", "tests/fixtures/tiny"],
        "stdout": "plain 1110 header 65 documents 94 dictionary 231 postings 456 positions 176\ndelta 1110 header 65 documents 94 dictionary 231 postings 456 positions 176\nvarbyte 403 header 65 documents 94 dictionary 77 postings 57 positions 22\nratio 2.75",
    },
    {
        "name": "a missing source is reported",
        "argv": ["index-size", "tests/fixtures/nope"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a corrupt index cannot be measured",
        "argv": ["index-size", "tests/fixtures/index/truncated.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "index-size with no source is rejected",
        "argv": ["index-size"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
