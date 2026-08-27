"""`search stem-info <word>` -- the Porter stemmer's consonant pattern and measure."""

NAME = "Porter measure"
ORDER = 60


def info(pattern, measure, vowel, double, cvc):
    return (f"pattern: {pattern}\n"
            f"measure: {measure}\n"
            f"vowel: {vowel}\n"
            f"double: {double}\n"
            f"cvc: {cvc}")


CASES = [
    # --- the paper's own m = 0 examples ---
    {
        "name": "tr: two consonants, no vowel, m = 0",
        "argv": ["stem-info", "tr"],
        "stdout": info("CC", 0, "no", "no", "no"),
    },
    {
        "name": "ee: two vowels, m = 0, and matching vowels are not a double consonant",
        "argv": ["stem-info", "ee"],
        "stdout": info("VV", 0, "yes", "no", "no"),
    },
    {
        "name": "tree: m = 0",
        "argv": ["stem-info", "tree"],
        "stdout": info("CCVV", 0, "yes", "no", "no"),
    },
    {
        "name": "y alone is a consonant, so the word has no vowel",
        "argv": ["stem-info", "y"],
        "stdout": info("C", 0, "no", "no", "no"),
    },
    {
        "name": "by: y after a consonant becomes a vowel",
        "argv": ["stem-info", "by"],
        "stdout": info("CV", 0, "yes", "no", "no"),
    },

    # --- the paper's m = 1 examples ---
    {
        "name": "trouble: m = 1",
        "argv": ["stem-info", "trouble"],
        "stdout": info("CCVVCCV", 1, "yes", "no", "no"),
    },
    {
        "name": "oats: m = 1",
        "argv": ["stem-info", "oats"],
        "stdout": info("VVCC", 1, "yes", "no", "no"),
    },
    {
        "name": "trees: m = 1",
        "argv": ["stem-info", "trees"],
        "stdout": info("CCVVC", 1, "yes", "no", "no"),
    },
    {
        "name": "ivy: m = 1",
        "argv": ["stem-info", "ivy"],
        "stdout": info("VCV", 1, "yes", "no", "no"),
    },

    # --- the paper's m = 2 examples ---
    {
        "name": "troubles: m = 2, and it ends in cvc",
        "argv": ["stem-info", "troubles"],
        "stdout": info("CCVVCCVC", 2, "yes", "no", "yes"),
    },
    {
        "name": "private: m = 2",
        "argv": ["stem-info", "private"],
        "stdout": info("CCVCVCV", 2, "yes", "no", "no"),
    },
    {
        "name": "oaten: m = 2, and it ends in cvc",
        "argv": ["stem-info", "oaten"],
        "stdout": info("VVCVC", 2, "yes", "no", "yes"),
    },
    {
        "name": "orrery: m = 2, ending in a y that is a vowel",
        "argv": ["stem-info", "orrery"],
        "stdout": info("VCCVCV", 2, "yes", "no", "no"),
    },

    # --- y as a consonant ---
    {
        "name": "toy: y after a vowel stays a consonant",
        "argv": ["stem-info", "toy"],
        "stdout": info("CVC", 1, "yes", "no", "no"),
    },
    {
        "name": "syzygy: every y follows a consonant, so every y is a vowel",
        "argv": ["stem-info", "syzygy"],
        "stdout": info("CVCVCV", 2, "yes", "no", "no"),
    },

    # --- *d, the double consonant ---
    {
        "name": "hopp ends in a double consonant",
        "argv": ["stem-info", "hopp"],
        "stdout": info("CVCC", 1, "yes", "yes", "no"),
    },
    {
        "name": "hiss ends in a double consonant",
        "argv": ["stem-info", "hiss"],
        "stdout": info("CVCC", 1, "yes", "yes", "no"),
    },

    # --- *o, the cvc pattern ---
    {
        "name": "hop matches cvc",
        "argv": ["stem-info", "hop"],
        "stdout": info("CVC", 1, "yes", "no", "yes"),
    },
    {
        "name": "cat matches cvc",
        "argv": ["stem-info", "cat"],
        "stdout": info("CVC", 1, "yes", "no", "yes"),
    },
    {
        "name": "snow fails cvc because it ends in w",
        "argv": ["stem-info", "snow"],
        "stdout": info("CCVC", 1, "yes", "no", "no"),
    },
    {
        "name": "box fails cvc because it ends in x",
        "argv": ["stem-info", "box"],
        "stdout": info("CVC", 1, "yes", "no", "no"),
    },
    {
        "name": "fail fails cvc because the third-from-last letter is a vowel",
        "argv": ["stem-info", "fail"],
        "stdout": info("CVVC", 1, "yes", "no", "no"),
    },

    # --- degenerate and non-letter input ---
    {
        "name": "an empty word has no pattern and m = 0",
        "argv": ["stem-info", ""],
        "stdout": info("", 0, "no", "no", "no"),
    },
    {
        "name": "digits and punctuation count as consonants",
        "argv": ["stem-info", "3.14"],
        "stdout": info("CCCC", 0, "no", "no", "no"),
    },
]
