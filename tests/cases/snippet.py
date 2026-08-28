"""`search snippet` -- an excerpt of a document with the query terms marked."""

NAME = "Snippet generation"
ORDER = 400

CASES = [
    {
        "name": "matches are marked where the document spells them",
        "argv": ["snippet", "corpus/fixtures", "doc_001", "cats", "mat"],
        "stdout": "[Cats] and [Mats]\n\nThe [cat] sat on the [mat]. The [cat] was happy on that [mat]. A [mat] is a flat rug and the [cat] likes to sit on it every afternoon.",
    },
    {
        "name": "a narrow window slides to the match",
        "argv": ["snippet", "--max-chars", "60", "corpus/fixtures", "doc_001", "rug"],
        "stdout": "...the mat. The cat was happy on that mat. A mat is a flat [rug]...",
    },
    {
        "name": "the window covering the most distinct terms wins",
        "argv": ["snippet", "--max-chars", "70", "corpus/fixtures", "doc_001", "rug", "afternoon"],
        "stdout": "...A mat is a flat [rug] and the cat likes to sit on it every [afternoon].",
    },
    {
        "name": "punctuation stays outside the marks",
        "argv": ["snippet", "--max-chars", "40", "corpus/fixtures", "doc_001", "mat"],
        "stdout": "Cats and [Mats]\n\nThe cat sat on the [mat]....",
    },
    {
        "name": "a query word is stemmed before matching, like the index",
        "argv": ["snippet", "--max-chars", "60", "corpus/fixtures", "doc_001", "sitting"],
        "stdout": "...on that mat. A mat is a flat rug and the cat likes to [sit] on...",
    },
    {
        "name": "a document with no match yields its opening",
        "argv": ["snippet", "--max-chars", "50", "corpus/fixtures", "doc_001", "elephant"],
        "stdout": "Cats and Mats\n\nThe cat sat on the mat. The cat was...",
    },
    {
        "name": "a stopword query matches nothing and yields the opening",
        "argv": ["snippet", "--max-chars", "50", "corpus/fixtures", "doc_001", "the"],
        "stdout": "Cats and Mats\n\nThe cat sat on the mat. The cat was...",
    },
    {
        "name": "a match in the title is found there",
        "argv": ["snippet", "--max-chars", "40", "corpus/fixtures", "doc_026", "sun"],
        "stdout": "The [Sun]\n\nThe [sun] is a star at the centre...",
    },
    {
        "name": "another document",
        "argv": ["snippet", "--max-chars", "80", "corpus/fixtures", "doc_011", "merge", "sort"],
        "stdout": "[Sorting] Algorithms\n\n[Sorting] an array is a common task. The [merge] [sort] algorithm...",
    },
    {
        "name": "a missing document is reported",
        "argv": ["snippet", "corpus/fixtures", "doc_999", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing corpus is reported",
        "argv": ["snippet", "tests/fixtures/nope", "doc_001", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a zero window is rejected",
        "argv": ["snippet", "--max-chars", "0", "corpus/fixtures", "doc_001", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "snippet with no query word is rejected",
        "argv": ["snippet", "corpus/fixtures", "doc_001"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
