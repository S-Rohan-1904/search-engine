"""`search show <dir> <id>` -- reading and parsing a single document."""

NAME = "Document parsing"
ORDER = 20

DOCS = "tests/fixtures/docs"

CASES = [
    {
        "name": "prints the id, title and body of a corpus document",
        "argv": ["show", "corpus/fixtures", "doc_001"],
        "stdout": (
            "id: doc_001\n"
            "title: Cats and Mats\n"
            "\n"
            "The cat sat on the mat. The cat was happy on that mat. A mat is a "
            "flat rug and the cat likes to sit on it every afternoon."
        ),
    },
    {
        "name": "preserves blank lines inside the body",
        "argv": ["show", DOCS, "multi_para"],
        "stdout": (
            "id: multi_para\n"
            "title: Two Paragraphs\n"
            "\n"
            "The first paragraph talks about the index.\n"
            "\n"
            "The second paragraph talks about the query."
        ),
    },
    {
        "name": "trims whitespace around the title value",
        "argv": ["show", DOCS, "spaced_title"],
        "stdout": (
            "id: spaced_title\n"
            "title: Lots Of Space\n"
            "\n"
            "The body."
        ),
    },
    {
        "name": "splits the header on the first colon only",
        "argv": ["show", DOCS, "colon_title"],
        "stdout": (
            "id: colon_title\n"
            "title: Ratio: 3:1\n"
            "\n"
            "Aspect ratios and colons."
        ),
    },
    {
        "name": "accepts a document with an empty body",
        "argv": ["show", DOCS, "empty_body"],
        "stdout": (
            "id: empty_body\n"
            "title: Nothing Here"
        ),
    },
    {
        "name": "accepts an empty title value",
        "argv": ["show", DOCS, "empty_title"],
        "stdout": (
            "id: empty_title\n"
            "title:\n"
            "\n"
            "Body with no title."
        ),
    },
    {
        "name": "strips blank and whitespace-only padding around the body",
        "argv": ["show", DOCS, "extra_blanks"],
        "stdout": (
            "id: extra_blanks\n"
            "title: Padded\n"
            "\n"
            "The body starts here."
        ),
    },
    {
        "name": "does not require a blank line after the header",
        "argv": ["show", DOCS, "no_blank_line"],
        "stdout": (
            "id: no_blank_line\n"
            "title: Tight\n"
            "\n"
            "Body immediately after."
        ),
    },
    {
        "name": "rejects a document with no header line",
        "argv": ["show", DOCS, "no_title"],
        "stdout": "",
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "rejects a header whose key is not 'title'",
        "argv": ["show", DOCS, "wrong_key"],
        "stdout": "",
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "rejects an empty file",
        "argv": ["show", DOCS, "empty"],
        "stdout": "",
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "exits 1 with a message on stderr for an unknown document id",
        "argv": ["show", "corpus/fixtures", "doc_999"],
        "stdout": "",
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "exits 1 with a message on stderr when the corpus is missing",
        "argv": ["show", "corpus/does_not_exist", "doc_001"],
        "stdout": "",
        "exit_code": 1,
        "stderr_not_empty": True,
    },
]
