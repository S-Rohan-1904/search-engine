"""`search bench` -- measuring build and query performance. Only run-to-run stable figures are asserted; timings and memory are not."""

NAME = "Benchmark harness"
ORDER = 460

CASES = [
    {
        "name": "build reports the shape of the index it built",
        "argv": ["bench", "build", "--stable", "corpus/fixtures"],
        "stdout": "source corpus/fixtures\nsource_bytes 4361\nthreads 1\ndocuments 30\nterms 235\npostings 343\nindex_bytes 3658",
    },
    {
        "name": "the tiny fixture",
        "argv": ["bench", "build", "--stable", "tests/fixtures/tiny"],
        "stdout": "source tests/fixtures/tiny\nsource_bytes 199\nthreads 1\ndocuments 3\nterms 11\npostings 19\nindex_bytes 215",
    },
    {
        "name": "an empty corpus measures as empty",
        "argv": ["bench", "build", "--stable", "tests/fixtures/empty"],
        "stdout": "source tests/fixtures/empty\nsource_bytes 0\nthreads 1\ndocuments 0\nterms 0\npostings 0\nindex_bytes 11",
    },
    {
        "name": "thread count is reported back",
        "argv": ["bench", "build", "--stable", "--threads", "4", "corpus/fixtures"],
        "stdout": "source corpus/fixtures\nsource_bytes 4361\nthreads 4\ndocuments 30\nterms 235\npostings 343\nindex_bytes 3658",
    },
    {
        "name": "more threads do not change what was built",
        "argv": ["bench", "build", "--stable", "--threads", "8", "corpus/fixtures"],
        "stdout": "source corpus/fixtures\nsource_bytes 4361\nthreads 8\ndocuments 30\nterms 235\npostings 343\nindex_bytes 3658",
    },
    {
        "name": "query reports how many queries ran and what they returned",
        "argv": ["bench", "query", "--stable", "corpus/fixtures", "tests/fixtures/queries/basic.txt"],
        "stdout": "source corpus/fixtures\nsource_bytes 4361\ndocuments 30\nqueries 30\nrepeats 1\nlimit 10\nresults_returned 67\nqueries_rejected 0",
    },
    {
        "name": "repeats do not change the results counted",
        "argv": ["bench", "query", "--stable", "--repeats", "5", "corpus/fixtures", "tests/fixtures/queries/basic.txt"],
        "stdout": "source corpus/fixtures\nsource_bytes 4361\ndocuments 30\nqueries 30\nrepeats 5\nlimit 10\nresults_returned 67\nqueries_rejected 0",
    },
    {
        "name": "the result limit is reported and applied",
        "argv": ["bench", "query", "--stable", "--limit", "3", "corpus/fixtures", "tests/fixtures/queries/basic.txt"],
        "stdout": "source corpus/fixtures\nsource_bytes 4361\ndocuments 30\nqueries 30\nrepeats 1\nlimit 3\nresults_returned 46\nqueries_rejected 0",
    },
    {
        "name": "a limit of zero returns nothing",
        "argv": ["bench", "query", "--stable", "--limit", "0", "corpus/fixtures", "tests/fixtures/queries/basic.txt"],
        "stdout": "source corpus/fixtures\nsource_bytes 4361\ndocuments 30\nqueries 30\nrepeats 1\nlimit 0\nresults_returned 0\nqueries_rejected 0",
    },
    {
        "name": "querying the tiny fixture",
        "argv": ["bench", "query", "--stable", "tests/fixtures/tiny", "tests/fixtures/queries/basic.txt"],
        "stdout": "source tests/fixtures/tiny\nsource_bytes 199\ndocuments 3\nqueries 30\nrepeats 1\nlimit 10\nresults_returned 13\nqueries_rejected 0",
    },
    {
        "name": "a saved index can be queried",
        "argv": ["bench", "query", "--stable", "tests/fixtures/index/good.bin", "tests/fixtures/queries/basic.txt"],
        "stdout": "source tests/fixtures/index/good.bin\nsource_bytes 215\ndocuments 3\nqueries 30\nrepeats 1\nlimit 10\nresults_returned 13\nqueries_rejected 0",
    },
    {
        # A corpus file used to fall through to the index loader, which failed
        # silently and left an empty index to benchmark: 30 queries, zero
        # results, microsecond latencies that looked like a very fast engine.
        "name": "a corpus file is indexed rather than benchmarked empty",
        "argv": ["bench", "query", "--stable", "tests/fixtures/corpusfile/tiny.corpus", "tests/fixtures/queries/basic.txt"],
        "stdout": "source tests/fixtures/corpusfile/tiny.corpus\nsource_bytes 297\ndocuments 3\nqueries 30\nrepeats 1\nlimit 10\nresults_returned 13\nqueries_rejected 0",
    },
    {
        "name": "a source that yields no index is rejected rather than benchmarked",
        "argv": ["bench", "query", "tests/fixtures/corpusfile/empty.corpus", "tests/fixtures/queries/basic.txt"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "flags may come before or after the subcommand",
        "argv": ["bench", "--stable", "query", "corpus/fixtures", "tests/fixtures/queries/basic.txt"],
        "stdout": "source corpus/fixtures\nsource_bytes 4361\ndocuments 30\nqueries 30\nrepeats 1\nlimit 10\nresults_returned 67\nqueries_rejected 0",
    },
    {
        "name": "an unknown benchmark is rejected",
        "argv": ["bench", "nonsense", "corpus/fixtures"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing source is reported",
        "argv": ["bench", "build", "tests/fixtures/nope"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "building from a saved index is rejected, since it has no documents to read",
        "argv": ["bench", "build", "tests/fixtures/index/good.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "zero threads is rejected",
        "argv": ["bench", "build", "--threads", "0", "corpus/fixtures"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "zero repeats is rejected",
        "argv": ["bench", "query", "--repeats", "0", "corpus/fixtures", "tests/fixtures/queries/basic.txt"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a non-numeric flag value is rejected",
        "argv": ["bench", "build", "--threads", "x", "corpus/fixtures"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unknown flag is rejected",
        "argv": ["bench", "build", "--bogus", "1", "corpus/fixtures"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing query file is reported",
        "argv": ["bench", "query", "corpus/fixtures", "tests/fixtures/nope.txt"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "bench with no source is rejected",
        "argv": ["bench", "build"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
