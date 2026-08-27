"""`search docs <dir>` -- discovering the documents in a corpus directory."""

NAME = "Corpus listing"
ORDER = 10

FIXTURE_IDS = "\n".join(f"doc_{i:03d}" for i in range(1, 31))

CASES = [
    {
        "name": "lists all 30 documents in the fixture corpus, sorted",
        "argv": ["docs", "corpus/fixtures"],
        "stdout": FIXTURE_IDS,
    },
    {
        "name": "strips the .txt extension to form the document id",
        "argv": ["docs", "tests/fixtures/tiny"],
        "stdout": "alpha\nbeta\ngamma",
    },
    {
        "name": "ignores non-.txt files and does not recurse into subdirectories",
        # tests/fixtures/tiny also holds notes.md and nested/buried.txt,
        # neither of which is a document.
        "argv": ["docs", "tests/fixtures/tiny"],
        "stdout": "alpha\nbeta\ngamma",
    },
    {
        "name": "sorts ids rather than trusting directory order",
        "argv": ["docs", "tests/fixtures/unsorted"],
        "stdout": "alpha\ndelta\nmike\nzulu",
    },
    {
        "name": "prints nothing for a directory with no documents",
        "argv": ["docs", "tests/fixtures/empty"],
        "stdout": "",
        "exit_code": 0,
    },
    {
        "name": "exits 1 with a message on stderr when the directory is missing",
        "argv": ["docs", "corpus/does_not_exist"],
        "stdout": "",
        "exit_code": 1,
        "stderr_not_empty": True,
    },
]
