"""`search lengths <corpus_dir>` -- indexed document lengths and their average."""

NAME = "Document statistics"
ORDER = 220

CASES = [
    {
        "name": "each document reports the length of its analyzed term stream",
        "argv": ["lengths", "tests/fixtures/tiny"],
        "stdout": "alpha 7\nbeta 7\ngamma 8\naverage 7.33",
    },
    {
        "name": "lengths across the working corpus",
        "argv": ["lengths", "corpus/fixtures"],
        "stdout": "doc_001 16\ndoc_002 15\ndoc_003 16\ndoc_004 19\ndoc_005 20\ndoc_006 15\ndoc_007 15\ndoc_008 17\ndoc_009 15\ndoc_010 14\ndoc_011 16\ndoc_012 17\ndoc_013 14\ndoc_014 15\ndoc_015 15\ndoc_016 14\ndoc_017 18\ndoc_018 17\ndoc_019 16\ndoc_020 16\ndoc_021 13\ndoc_022 15\ndoc_023 16\ndoc_024 14\ndoc_025 17\ndoc_026 13\ndoc_027 15\ndoc_028 14\ndoc_029 15\ndoc_030 15\naverage 15.57",
    },
    {
        "name": "an empty corpus has an average of zero",
        "argv": ["lengths", "tests/fixtures/empty"],
        "stdout": "average 0.00",
    },
    {
        "name": "malformed documents contribute no length",
        "argv": ["lengths", "tests/fixtures/docs"],
        "stdout": "colon_title 5\nempty_body 1\nempty_title 2\nextra_blanks 3\nmulti_para 10\nno_blank_line 3\nspaced_title 3\naverage 3.86",
    },
    {
        "name": "documents appear in the order they were indexed",
        "argv": ["lengths", "tests/fixtures/unsorted"],
        "stdout": "alpha 5\ndelta 5\nmike 5\nzulu 5\naverage 5.00",
    },
    {
        "name": "a missing corpus directory is reported",
        "argv": ["lengths", "tests/fixtures/nope"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "lengths with no directory is rejected",
        "argv": ["lengths"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
