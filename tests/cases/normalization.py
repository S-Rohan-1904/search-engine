"""`search normalize <token>` and `search terms <text>` -- reducing tokens to terms."""

NAME = "Normalization"
ORDER = 40

CASES = [
    {
        "name": "folds ASCII uppercase to lowercase",
        "argv": ["normalize", "The"],
        "stdout": "the",
    },
    {
        "name": "strips trailing punctuation",
        "argv": ["normalize", "mat."],
        "stdout": "mat",
    },
    {
        "name": "strips punctuation from both ends",
        "argv": ["normalize", "(hello)"],
        "stdout": "hello",
    },
    {
        "name": "keeps a hyphen between two word characters",
        "argv": ["normalize", "THE-END"],
        "stdout": "the-end",
    },
    {
        "name": "drops hyphens that have nothing on one side",
        "argv": ["normalize", "-cat-"],
        "stdout": "cat",
    },
    {
        "name": "removes apostrophes so a contraction closes up",
        "argv": ["normalize", "don't"],
        "stdout": "dont",
    },
    {
        "name": "removes a trailing possessive apostrophe",
        "argv": ["normalize", "cats'"],
        "stdout": "cats",
    },
    {
        "name": "keeps digits",
        "argv": ["normalize", "42"],
        "stdout": "42",
    },
    {
        "name": "keeps a decimal point between two digits",
        "argv": ["normalize", "3.14"],
        "stdout": "3.14",
    },
    {
        "name": "collapses the periods in an acronym",
        "argv": ["normalize", "U.S.A."],
        "stdout": "usa",
    },
    {
        "name": "keeps non-ASCII bytes verbatim while folding the ASCII around them",
        "argv": ["normalize", "Café,"],
        "stdout": "café",
    },
    {
        "name": "drops a token made only of punctuation",
        "argv": ["normalize", "..."],
        "stdout": "",
    },
    {
        "name": "drops an empty token",
        "argv": ["normalize", ""],
        "stdout": "",
    },
    {
        "name": "runs the whole pipeline and leaves a gap where a token was dropped",
        # "..." is token 2; it normalizes away, so position 2 never appears.
        "argv": ["terms", "The cat, ... sat!"],
        "stdout": (
            "0 0 the\n"
            "1 4 cat\n"
            "3 13 sat"
        ),
    },
    {
        "name": "preserves original offsets through the pipeline",
        "argv": ["terms", "State-of-the-art U.S.A. costs 3.14 dollars"],
        "stdout": (
            "0 0 state-of-the-art\n"
            "1 17 usa\n"
            "2 24 costs\n"
            "3 30 3.14\n"
            "4 35 dollars"
        ),
    },
    {
        "name": "produces no terms for input that is entirely punctuation",
        "argv": ["terms", "... !!! ---"],
        "stdout": "",
        "exit_code": 0,
    },
]
