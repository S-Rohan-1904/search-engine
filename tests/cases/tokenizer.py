"""`search tokenize <text>` -- splitting text into positioned tokens."""

NAME = "Tokenizer"
ORDER = 30

CASES = [
    {
        "name": "splits a sentence into tokens with positions and byte offsets",
        "argv": ["tokenize", "The cat sat on the mat."],
        "stdout": (
            "0 0 The\n"
            "1 4 cat\n"
            "2 8 sat\n"
            "3 12 on\n"
            "4 15 the\n"
            "5 19 mat."
        ),
    },
    {
        "name": "ignores leading and trailing whitespace",
        "argv": ["tokenize", "   hello   world   "],
        "stdout": (
            "0 3 hello\n"
            "1 11 world"
        ),
    },
    {
        "name": "treats a run of whitespace as one separator",
        "argv": ["tokenize", "a  b   c"],
        "stdout": (
            "0 0 a\n"
            "1 3 b\n"
            "2 7 c"
        ),
    },
    {
        "name": "separates on tabs, newlines and carriage returns",
        "argv": ["tokenize", "one\ttwo\nthree\r\nfour"],
        "stdout": (
            "0 0 one\n"
            "1 4 two\n"
            "2 8 three\n"
            "3 15 four"
        ),
    },
    {
        "name": "keeps punctuation, case and digits untouched",
        "argv": ["tokenize", "cat, dog. THE-END 42"],
        "stdout": (
            "0 0 cat,\n"
            "1 5 dog.\n"
            "2 10 THE-END\n"
            "3 18 42"
        ),
    },
    {
        "name": "reports byte offsets, not character offsets, for UTF-8 input",
        # 'é' is two bytes, so "au" starts at byte 6 but character 5.
        "argv": ["tokenize", "café au lait"],
        "stdout": (
            "0 0 café\n"
            "1 6 au\n"
            "2 9 lait"
        ),
    },
    {
        "name": "emits a single token when there is no whitespace",
        "argv": ["tokenize", "solitary"],
        "stdout": "0 0 solitary",
    },
    {
        "name": "produces no tokens for an empty input",
        "argv": ["tokenize", ""],
        "stdout": "",
        "exit_code": 0,
    },
    {
        "name": "produces no tokens for whitespace-only input",
        "argv": ["tokenize", "   \t \n  "],
        "stdout": "",
        "exit_code": 0,
    },
]
