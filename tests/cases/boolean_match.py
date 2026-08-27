"""`search match <corpus_dir> <query>` -- evaluating a parsed query."""

NAME = "Boolean query evaluation"
ORDER = 200

CASES = [
    {
        "name": "a bare word matches the documents containing it",
        "argv": ["match", "tests/fixtures/tiny", "cat"],
        "stdout": "alpha\ngamma",
    },
    {
        "name": "AND intersects",
        "argv": ["match", "tests/fixtures/tiny", "cat AND dog"],
        "stdout": "gamma",
    },
    {
        "name": "two adjacent words mean AND",
        "argv": ["match", "tests/fixtures/tiny", "cat dog"],
        "stdout": "gamma",
    },
    {
        "name": "OR unions",
        "argv": ["match", "tests/fixtures/tiny", "cat OR dog"],
        "stdout": "alpha\nbeta\ngamma",
    },
    {
        "name": "NOT complements against the whole corpus",
        "argv": ["match", "tests/fixtures/tiny", "NOT cat"],
        "stdout": "beta",
    },
    {
        "name": "NOT of a word matching nothing returns every document",
        "argv": ["match", "tests/fixtures/tiny", "NOT zzz"],
        "stdout": "alpha\nbeta\ngamma",
    },
    {
        "name": "AND binds tighter than OR",
        "argv": ["match", "corpus/fixtures", "oven OR cat AND mat"],
        "stdout": "doc_001\ndoc_018",
    },
    {
        "name": "parentheses change the result",
        "argv": ["match", "corpus/fixtures", "(oven OR cat) AND mat"],
        "stdout": "doc_001",
    },
    {
        "name": "NOT inside AND excludes",
        "argv": ["match", "tests/fixtures/tiny", "cat AND NOT dog"],
        "stdout": "alpha",
    },
    {
        "name": "double negation returns the original set",
        "argv": ["match", "corpus/fixtures", "NOT NOT cat"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "the query word is analyzed like the corpus was",
        "argv": ["match", "tests/fixtures/tiny", "Cats"],
        "stdout": "alpha\ngamma",
    },
    {
        "name": "a stemmed query form finds the same documents",
        "argv": ["match", "tests/fixtures/tiny", "mentioning"],
        "stdout": "alpha\nbeta\ngamma",
    },
    {
        "name": "a stopword carries no constraint, so this means cat alone",
        "argv": ["match", "tests/fixtures/tiny", "the cat"],
        "stdout": "alpha\ngamma",
    },
    {
        "name": "NOT of a stopword excludes nothing",
        "argv": ["match", "tests/fixtures/tiny", "cat AND NOT the"],
        "stdout": "alpha\ngamma",
    },
    {
        "name": "a query of only stopwords matches nothing",
        "argv": ["match", "tests/fixtures/tiny", "the of and"],
        "stdout": "",
    },
    {
        "name": "an unknown word matches nothing",
        "argv": ["match", "tests/fixtures/tiny", "elephant"],
        "stdout": "",
    },
    {
        "name": "nesting all three operators",
        "argv": ["match", "corpus/fixtures", "cat AND (sat OR happy) AND NOT oven"],
        "stdout": "doc_001",
    },
    {
        "name": "NOT applies to the whole parenthesized group",
        "argv": ["match", "corpus/fixtures", "cat AND NOT (mat OR sat)"],
        "stdout": "doc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "a group inside NOT inside AND",
        "argv": ["match", "corpus/fixtures", "(cat OR dog) AND NOT mat"],
        "stdout": "doc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "results are ascending by document id",
        "argv": ["match", "corpus/fixtures", "cat OR oven OR planet"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008\ndoc_018\ndoc_025\ndoc_026\ndoc_029",
    },
    {
        "name": "an empty corpus matches nothing",
        "argv": ["match", "tests/fixtures/empty", "cat OR dog"],
        "stdout": "",
    },
    {
        "name": "match agrees with the and command",
        "argv": ["match", "corpus/fixtures", "cat AND mat"],
        "stdout": "doc_001",
    },
    {
        "name": "match agrees with the or command",
        "argv": ["match", "corpus/fixtures", "cat OR mat"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "match agrees with the andnot command",
        "argv": ["match", "corpus/fixtures", "cat AND NOT mat"],
        "stdout": "doc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "a missing corpus directory is reported",
        "argv": ["match", "tests/fixtures/nope", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a malformed query is reported",
        "argv": ["match", "corpus/fixtures", "(cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an empty query is reported",
        "argv": ["match", "corpus/fixtures", ""],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "match with no query is rejected",
        "argv": ["match", "corpus/fixtures"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
    {
        "name": "an over-deep query is rejected before it can be evaluated",
        "argv": ["match", "corpus/fixtures", "(" * 300 + "cat" + ")" * 300],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
]
