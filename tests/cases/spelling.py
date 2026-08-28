"""`search suggest` -- dictionary terms near a misspelling, found through a BK-tree."""

NAME = "Spelling correction"
ORDER = 420

CASES = [
    {
        "name": "a transposition finds the intended word",
        "argv": ["suggest", "corpus/fixtures", "cta"],
        "stdout": "cat 2 7\nstar 2 4\ndata 2 3\ncut 2 2",
    },
    {
        "name": "a missing letter",
        "argv": ["suggest", "corpus/fixtures", "planetX"],
        "stdout": "planet 1 3",
    },
    {
        "name": "an exact term suggests itself first",
        "argv": ["suggest", "corpus/fixtures", "cat"],
        "stdout": "cat 0 7\ncut 1 2\neat 1 1\nmat 1 1\nsat 1 1",
    },
    {
        "name": "distance one only",
        "argv": ["suggest", "--max-distance", "1", "corpus/fixtures", "cta"],
        "stdout": "",
    },
    {
        "name": "distance zero finds only the term itself",
        "argv": ["suggest", "--max-distance", "0", "corpus/fixtures", "cat"],
        "stdout": "cat 0 7",
    },
    {
        "name": "the limit caps the list",
        "argv": ["suggest", "--limit", "2", "corpus/fixtures", "cta"],
        "stdout": "cat 2 7\nstar 2 4",
    },
    {
        "name": "a limit of zero returns nothing",
        "argv": ["suggest", "--limit", "0", "corpus/fixtures", "cta"],
        "stdout": "",
    },
    {
        "name": "ties are broken by document frequency",
        "argv": ["suggest", "--max-distance", "1", "corpus/fixtures", "mat"],
        "stdout": "mat 0 1\ncat 1 7\nmap 1 2\neat 1 1\nmar 1 1",
    },
    {
        "name": "a word unlike anything indexed suggests nothing",
        "argv": ["suggest", "corpus/fixtures", "zzzzzzzz"],
        "stdout": "",
    },
    {
        "name": "suggestions come from a saved index too",
        "argv": ["suggest", "tests/fixtures/index/good.bin", "cta"],
        "stdout": "cat 2 2\nbeta 2 1",
    },
    {
        "name": "the query word is analyzed first, so a plural finds its stem",
        "argv": ["suggest", "--max-distance", "0", "corpus/fixtures", "cats"],
        "stdout": "cat 0 7",
    },
    {
        "name": "a missing source is reported",
        "argv": ["suggest", "tests/fixtures/nope", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a multi-word argument is rejected",
        "argv": ["suggest", "corpus/fixtures", "cat dog"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "suggest with no word is rejected",
        "argv": ["suggest", "corpus/fixtures"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
