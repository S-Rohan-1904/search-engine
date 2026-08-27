"""`search phrase <corpus_dir> <text>` and quoted phrases inside `match`."""

NAME = "Phrase queries"
ORDER = 210

CASES = [
    {
        "name": "consecutive words match",
        "argv": ["phrase", "corpus/fixtures", "cat sat"],
        "stdout": "doc_001",
    },
    {
        "name": "the same words reversed do not",
        "argv": ["phrase", "corpus/fixtures", "sat cat"],
        "stdout": "",
    },
    {
        "name": "stopwords inside a phrase still hold their place",
        "argv": ["phrase", "corpus/fixtures", "sat on the mat"],
        "stdout": "doc_001",
    },
    {
        "name": "a leading stopword does not shift the rest",
        "argv": ["phrase", "corpus/fixtures", "the cat sat"],
        "stdout": "doc_001",
    },
    {
        "name": "three stopwords apart is not the same as adjacent",
        "argv": ["phrase", "corpus/fixtures", "sun is a star"],
        "stdout": "doc_026",
    },
    {
        "name": "dropping the gap words changes the phrase",
        "argv": ["phrase", "corpus/fixtures", "sun star"],
        "stdout": "",
    },
    {
        "name": "a phrase from another topic",
        "argv": ["phrase", "corpus/fixtures", "merge sort"],
        "stdout": "doc_011",
    },
    {
        "name": "another, spanning a stopword",
        "argv": ["phrase", "corpus/fixtures", "solar system"],
        "stdout": "doc_026",
    },
    {
        "name": "word order is what distinguishes it",
        "argv": ["phrase", "corpus/fixtures", "system solar"],
        "stdout": "",
    },
    {
        "name": "a phrase matching every document",
        "argv": ["phrase", "tests/fixtures/tiny", "tiny document"],
        "stdout": "alpha\nbeta\ngamma",
    },
    {
        "name": "a wrong gap between the words fails",
        "argv": ["phrase", "tests/fixtures/tiny", "tiny a document"],
        "stdout": "",
    },
    {
        "name": "a leading stopword in the query is fine",
        "argv": ["phrase", "tests/fixtures/tiny", "a tiny document"],
        "stdout": "alpha\nbeta\ngamma",
    },
    {
        "name": "phrase words are analyzed like the corpus",
        "argv": ["phrase", "tests/fixtures/tiny", "TINY DOCUMENTS"],
        "stdout": "alpha\nbeta\ngamma",
    },
    {
        "name": "a one-word phrase is just that word",
        "argv": ["phrase", "corpus/fixtures", "cats"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "an unknown word matches nothing",
        "argv": ["phrase", "corpus/fixtures", "elephant parade"],
        "stdout": "",
    },
    {
        "name": "a phrase combines with AND",
        "argv": ["match", "corpus/fixtures", "\"cat sat\" AND rug"],
        "stdout": "doc_001",
    },
    {
        "name": "a phrase combines with OR",
        "argv": ["match", "corpus/fixtures", "\"cat sat\" OR oven"],
        "stdout": "doc_001\ndoc_018",
    },
    {
        "name": "a phrase can be negated",
        "argv": ["match", "corpus/fixtures", "NOT \"cat sat\" AND cat"],
        "stdout": "doc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "a phrase of only stopwords carries no constraint",
        "argv": ["match", "corpus/fixtures", "\"the of\" AND cat"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "an empty phrase carries no constraint",
        "argv": ["match", "corpus/fixtures", "\"\" AND cat"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "operator words inside a phrase are literal",
        "argv": ["match", "corpus/fixtures", "\"cat AND sat\""],
        "stdout": "",
    },
    {
        "name": "an empty corpus matches no phrase",
        "argv": ["phrase", "tests/fixtures/empty", "cat sat"],
        "stdout": "",
    },
    {
        "name": "a quote inside a phrase argument is rejected",
        "argv": ["phrase", "corpus/fixtures", "a \" b"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing corpus directory is reported",
        "argv": ["phrase", "tests/fixtures/nope", "cat sat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "phrase with no text is rejected",
        "argv": ["phrase", "corpus/fixtures"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
