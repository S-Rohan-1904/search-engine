"""`search complete` -- dictionary terms beginning with a prefix, from a trie."""

NAME = "Autocomplete"
ORDER = 430

CASES = [
    {
        "name": "a short prefix, most common first",
        "argv": ["complete", "corpus/fixtures", "ca"],
        "stdout": "cat 7\ncare 1\ncarrot 1",
    },
    {
        "name": "a longer prefix narrows it",
        "argv": ["complete", "corpus/fixtures", "cat"],
        "stdout": "cat 7",
    },
    {
        "name": "a single letter",
        "argv": ["complete", "--limit", "5", "corpus/fixtures", "s"],
        "stdout": "small 4\nstar 4\nsimmer 2\nslow 2\nstart 2",
    },
    {
        "name": "the limit caps the list",
        "argv": ["complete", "--limit", "2", "corpus/fixtures", "s"],
        "stdout": "small 4\nstar 4",
    },
    {
        "name": "a limit of zero returns nothing",
        "argv": ["complete", "--limit", "0", "corpus/fixtures", "s"],
        "stdout": "",
    },
    {
        "name": "a prefix nothing starts with returns nothing",
        "argv": ["complete", "corpus/fixtures", "zzz"],
        "stdout": "",
    },
    {
        "name": "an empty prefix offers the whole dictionary, capped",
        "argv": ["complete", "--limit", "3", "corpus/fixtures", ""],
        "stdout": "cat 7\neveri 7\nwater 6",
    },
    {
        "name": "the prefix is normalized, so uppercase works",
        "argv": ["complete", "corpus/fixtures", "CA"],
        "stdout": "cat 7\ncare 1\ncarrot 1",
    },
    {
        "name": "completions come from a saved index too",
        "argv": ["complete", "tests/fixtures/index/good.bin", "d"],
        "stdout": "document 3\ndog 2",
    },
    {
        "name": "a prefix that is itself a term includes it",
        "argv": ["complete", "tests/fixtures/tiny", "cat"],
        "stdout": "cat 2",
    },
    {
        "name": "a missing source is reported",
        "argv": ["complete", "tests/fixtures/nope", "ca"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "complete with no prefix is rejected",
        "argv": ["complete", "corpus/fixtures"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
