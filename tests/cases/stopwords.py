"""`search stopword <word>` and `terms --drop-stopwords` -- filtering common terms."""

NAME = "Stopwords"
ORDER = 50

CASES = [
    {
        "name": "recognises a common function word",
        "argv": ["stopword", "the"],
        "stdout": "yes",
    },
    {
        "name": "leaves a content word alone",
        "argv": ["stopword", "cat"],
        "stdout": "no",
    },
    {
        "name": "matches regardless of the case it was typed in",
        "argv": ["stopword", "THE"],
        "stdout": "yes",
    },
    {
        "name": "matches a contraction after its apostrophe is normalized away",
        # The list spells it "don't"; an indexed term is spelled "dont".
        "argv": ["stopword", "don't"],
        "stdout": "yes",
    },
    {
        "name": "matches a capitalised contraction",
        "argv": ["stopword", "Don't"],
        "stdout": "yes",
    },
    {
        "name": "matches a contraction already written without its apostrophe",
        "argv": ["stopword", "youre"],
        "stdout": "yes",
    },
    {
        "name": "treats an empty term as not a stopword",
        "argv": ["stopword", ""],
        "stdout": "no",
    },
    {
        "name": "drops stopwords from a stream and leaves gaps behind",
        "argv": ["terms", "--drop-stopwords", "The cat sat on the mat."],
        "stdout": (
            "1 4 cat\n"
            "2 8 sat\n"
            "5 19 mat"
        ),
    },
    {
        "name": "drops a contraction that normalizes onto the list",
        "argv": ["terms", "--drop-stopwords", "Don't stop believing"],
        "stdout": (
            "1 6 stop\n"
            "2 11 believing"
        ),
    },
    {
        "name": "produces nothing when every term is a stopword",
        "argv": ["terms", "--drop-stopwords", "the of and a with"],
        "stdout": "",
        "exit_code": 0,
    },
    {
        "name": "keeps stopwords when the flag is absent",
        "argv": ["terms", "The cat sat on the mat."],
        "stdout": (
            "0 0 the\n"
            "1 4 cat\n"
            "2 8 sat\n"
            "3 12 on\n"
            "4 15 the\n"
            "5 19 mat"
        ),
    },
    {
        "name": "filters a sentence from the corpus",
        "argv": ["terms", "--drop-stopwords",
                 "A mat is a flat rug and the cat likes to sit on it"],
        "stdout": (
            "1 2 mat\n"
            "4 11 flat\n"
            "5 16 rug\n"
            "8 28 cat\n"
            "9 32 likes\n"
            "11 41 sit"
        ),
    },
]
