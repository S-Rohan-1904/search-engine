"""`search query [options] <source> <query>` -- ranked, formatted results."""

NAME = "Result formatting and query flags"
ORDER = 390

CASES = [
    {
        "name": "a boolean query is ranked, not just matched",
        "argv": ["query", "corpus/fixtures", "cat AND mat"],
        "stdout": "1. doc_001  7.4906",
    },
    {
        "name": "results are numbered from one",
        "argv": ["query", "--limit", "3", "corpus/fixtures", "cat"],
        "stdout": "1. doc_003  2.5076\n2. doc_001  2.3900\n3. doc_004  2.1294",
    },
    {
        "name": "the limit caps the list",
        "argv": ["query", "--limit", "1", "corpus/fixtures", "cat"],
        "stdout": "1. doc_003  2.5076",
    },
    {
        "name": "a limit of zero shows nothing",
        "argv": ["query", "--limit", "0", "corpus/fixtures", "cat"],
        "stdout": "",
    },
    {
        "name": "the scorer can be switched",
        "argv": ["query", "--limit", "3", "--scorer", "tfidf", "corpus/fixtures", "cat"],
        "stdout": "1. doc_003  3.7975\n2. doc_001  3.4727\n3. doc_004  3.0541",
    },
    {
        "name": "tsv output is id and score only",
        "argv": ["query", "--tsv", "--limit", "3", "corpus/fixtures", "cat"],
        "stdout": "doc_003\t2.5076\ndoc_001\t2.3900\ndoc_004\t2.1294",
    },
    {
        "name": "snippets appear under each result",
        "argv": ["query", "--limit", "2", "--snippet", "--max-chars", "70", "corpus/fixtures", "cat OR oven"],
        "stdout": "1. doc_018  2.9186\n   ...salt and yeast. The baker mixes the dough and bakes it in a hot [oven]...\n2. doc_003  2.5076\n   The Black [Cat] A black [cat] crossed the road at night. The black [cat]...",
    },
    {
        "name": "a snippet is flattened onto one line",
        "argv": ["query", "--limit", "1", "--snippet", "--max-chars", "60", "corpus/fixtures", "doc"],
        "stdout": "",
    },
    {
        "name": "a saved index ranks but cannot show snippets",
        "argv": ["query", "--snippet", "tests/fixtures/index/good.bin", "cat"],
        "stdout": "1. alpha  0.4789\n2. gamma  0.4532",
    },
    {
        "name": "a document matching only through NOT scores zero",
        "argv": ["query", "tests/fixtures/tiny", "NOT cat"],
        "stdout": "1. beta  0.0000",
    },
    {
        "name": "phrases work here too",
        "argv": ["query", "corpus/fixtures", "\"cat sat\""],
        "stdout": "1. doc_001  5.3844",
    },
    {
        "name": "a stopword query carries no constraint",
        "argv": ["query", "--limit", "2", "tests/fixtures/tiny", "the cat"],
        "stdout": "1. alpha  0.4789\n2. gamma  0.4532",
    },
    {
        "name": "a query matching nothing shows nothing",
        "argv": ["query", "corpus/fixtures", "elephant"],
        "stdout": "",
    },
    {
        "name": "an empty corpus shows nothing",
        "argv": ["query", "tests/fixtures/empty", "cat"],
        "stdout": "",
    },
    {
        "name": "ranking respects the boolean set, not just the terms",
        "argv": ["query", "--tsv", "corpus/fixtures", "cat AND NOT mat"],
        "stdout": "doc_003\t2.5076\ndoc_004\t2.1294\ndoc_002\t1.4405\ndoc_006\t1.4405\ndoc_008\t1.3676\ndoc_005\t1.2710",
    },
    {
        "name": "a malformed query is reported",
        "argv": ["query", "corpus/fixtures", "(cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unknown scorer is rejected",
        "argv": ["query", "--scorer", "bogus", "corpus/fixtures", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a zero snippet width is rejected",
        "argv": ["query", "--max-chars", "0", "corpus/fixtures", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unknown flag is rejected",
        "argv": ["query", "--bogus", "1", "corpus/fixtures", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing source is reported",
        "argv": ["query", "tests/fixtures/nope", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "query with no query string is rejected",
        "argv": ["query", "corpus/fixtures"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
