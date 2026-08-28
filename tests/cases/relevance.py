"""`search beir-import` and `search evaluate` -- scoring against judgements someone else wrote."""

NAME = "Relevance measurement"
ORDER = 530

CASES = [
    {
        "name": "a BEIR corpus imports with its own document ids",
        "argv": ["beir-import", "tests/fixtures/beir/corpus.jsonl", "build/beir-test.corpus"],
        "stdout": "lines 4\ndocuments 3\nskipped 1",
    },
    {
        "name": "the ids are BEIR's, not ordinals, because the qrels name them",
        "argv": ["docs", "build/beir-test.corpus"],
        "stdout": "d1\nd10\nd2",
    },
    {
        "name": "title and body are both indexed",
        "argv": ["show", "build/beir-test.corpus", "d1"],
        "stdout": "id: d1\ntitle: Cats\n\nCats sit on mats.",
    },
    {
        "name": "a record with neither title nor text is skipped",
        "argv": ["match", "build/beir-test.corpus", "empty"],
        "stdout": "",
    },
    {
        # Query 1 judges d1 relevant and it ranks first, so every measure is
        # perfect. Query 2 judges d10, which no query term reaches, so it
        # scores zero. The average of the two is what the command prints.
        "name": "the measures average over judged queries",
        "argv": ["evaluate", "build/beir-test.corpus",
                 "tests/fixtures/beir/queries.jsonl", "tests/fixtures/beir/qrels.tsv"],
        "stdout": "queries 2\nunjudged 1\nscorer bm25\nndcg_at_10 0.5000\n"
                  "precision_at_10 0.0500\nrecall_at_100 0.5000",
    },
    {
        "name": "a missing qrels file is reported",
        "argv": ["evaluate", "build/beir-test.corpus",
                 "tests/fixtures/beir/queries.jsonl", "tests/fixtures/beir/nope.tsv"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing queries file is reported",
        "argv": ["evaluate", "build/beir-test.corpus",
                 "tests/fixtures/beir/nope.jsonl", "tests/fixtures/beir/qrels.tsv"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "beir-import with no destination is rejected",
        "argv": ["beir-import", "tests/fixtures/beir/corpus.jsonl"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
