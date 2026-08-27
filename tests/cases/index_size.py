"""`search index-size <source>` -- what the encodings cost."""

NAME = "Index size report"
ORDER = 300

CASES = [
    {
        "name": "the working corpus, broken down by section",
        "argv": ["index-size", "corpus/fixtures"],
        "stdout": "plain 17570 header 9 documents 698 dictionary 4895 postings 8232 positions 3736\ndelta 17570 header 9 documents 698 dictionary 4895 postings 8232 positions 3736\nvarbyte 3375 header 9 documents 271 dictionary 1599 postings 1029 positions 467\nratio 5.21",
    },
    {
        "name": "the tiny fixture",
        "argv": ["index-size", "tests/fixtures/tiny"],
        "stdout": "plain 950 header 9 documents 70 dictionary 239 postings 456 positions 176\ndelta 950 header 9 documents 70 dictionary 239 postings 456 positions 176\nvarbyte 187 header 9 documents 21 dictionary 78 postings 57 positions 22\nratio 5.08",
    },
    {
        "name": "an empty corpus is almost all header",
        "argv": ["index-size", "tests/fixtures/empty"],
        "stdout": "plain 25 header 9 documents 8 dictionary 0 postings 0 positions 0\ndelta 25 header 9 documents 8 dictionary 0 postings 0 positions 0\nvarbyte 11 header 9 documents 1 dictionary 0 postings 0 positions 0\nratio 2.27",
    },
    {
        "name": "a saved index measures the same as the corpus it came from",
        "argv": ["index-size", "tests/fixtures/index/good.bin"],
        "stdout": "plain 950 header 9 documents 70 dictionary 239 postings 456 positions 176\ndelta 950 header 9 documents 70 dictionary 239 postings 456 positions 176\nvarbyte 187 header 9 documents 21 dictionary 78 postings 57 positions 22\nratio 5.08",
    },
    {
        "name": "and so does the corpus itself",
        "argv": ["index-size", "tests/fixtures/tiny"],
        "stdout": "plain 950 header 9 documents 70 dictionary 239 postings 456 positions 176\ndelta 950 header 9 documents 70 dictionary 239 postings 456 positions 176\nvarbyte 187 header 9 documents 21 dictionary 78 postings 57 positions 22\nratio 5.08",
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
