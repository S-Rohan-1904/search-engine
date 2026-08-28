"""`search edit-distance <a> <b>` -- how many edits separate two words."""

NAME = "Levenshtein distance"
ORDER = 410

CASES = [
    {
        "name": "the textbook example",
        "argv": ["edit-distance", "kitten", "sitting"],
        "stdout": "3",
    },
    {
        "name": "identical words are zero apart",
        "argv": ["edit-distance", "cat", "cat"],
        "stdout": "0",
    },
    {
        "name": "one insertion",
        "argv": ["edit-distance", "cat", "cats"],
        "stdout": "1",
    },
    {
        "name": "one deletion",
        "argv": ["edit-distance", "cats", "cat"],
        "stdout": "1",
    },
    {
        "name": "one substitution",
        "argv": ["edit-distance", "cat", "cot"],
        "stdout": "1",
    },
    {
        "name": "a transposition costs two, since Levenshtein has no swap",
        "argv": ["edit-distance", "form", "from"],
        "stdout": "2",
    },
    {
        "name": "nothing in common",
        "argv": ["edit-distance", "abc", "xyz"],
        "stdout": "3",
    },
    {
        "name": "against an empty string the distance is the length",
        "argv": ["edit-distance", "cat", ""],
        "stdout": "3",
    },
    {
        "name": "two empty strings",
        "argv": ["edit-distance", "", ""],
        "stdout": "0",
    },
    {
        "name": "the measure is symmetric",
        "argv": ["edit-distance", "sitting", "kitten"],
        "stdout": "3",
    },
    {
        "name": "a longer pair",
        "argv": ["edit-distance", "intention", "execution"],
        "stdout": "5",
    },
    {
        "name": "edit-distance with one word is rejected",
        "argv": ["edit-distance", "cat"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
