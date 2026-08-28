"""`search pagerank <links_file>` -- importance from the link graph."""

NAME = "PageRank"
ORDER = 450

CASES = [
    {
        "name": "a mutual pair splits the score evenly",
        "argv": ["pagerank", "tests/fixtures/graph/cycle.tsv"],
        "stdout": "a 0.500000\nb 0.500000",
    },
    {
        "name": "a star gives the hub the most",
        "argv": ["pagerank", "tests/fixtures/graph/star.tsv"],
        "stdout": "a 0.541984\nb 0.152672\nc 0.152672\nd 0.152672",
    },
    {
        "name": "a chain accumulates toward the end",
        "argv": ["pagerank", "tests/fixtures/graph/chain.tsv"],
        "stdout": "c 0.474412\nb 0.341171\na 0.184417",
    },
    {
        "name": "a dangling page's score is redistributed, not lost",
        "argv": ["pagerank", "tests/fixtures/graph/dangling.tsv"],
        "stdout": "b 0.649123\na 0.350877",
    },
    {
        "name": "a repeated link counts once",
        "argv": ["pagerank", "tests/fixtures/graph/multi.tsv"],
        "stdout": "b 0.370130\nc 0.370130\na 0.259740",
    },
    {
        "name": "an empty graph ranks nothing",
        "argv": ["pagerank", "tests/fixtures/graph/empty.tsv"],
        "stdout": "",
    },
    {
        "name": "fewer iterations have not converged yet",
        "argv": ["pagerank", "--iterations", "1", "tests/fixtures/graph/star.tsv"],
        "stdout": "a 0.728125\nb 0.090625\nc 0.090625\nd 0.090625",
    },
    {
        "name": "zero iterations leave every page equal",
        "argv": ["pagerank", "--iterations", "0", "tests/fixtures/graph/star.tsv"],
        "stdout": "a 0.250000\nb 0.250000\nc 0.250000\nd 0.250000",
    },
    {
        "name": "damping of zero makes every page equal",
        "argv": ["pagerank", "--damping", "0", "tests/fixtures/graph/star.tsv"],
        "stdout": "a 0.250000\nb 0.250000\nc 0.250000\nd 0.250000",
    },
    {
        "name": "a crawl writes a link graph it can rank",
        "argv": ["pagerank", "build/pr/links.tsv"],
        "stdout": "doc_0002 0.412590\ndoc_0004 0.412590\ndoc_0003 0.061889\ndoc_0005 0.061889\ndoc_0001 0.051042",
    },
    {
        "name": "a missing links file is reported",
        "argv": ["pagerank", "tests/fixtures/nope.tsv"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a non-numeric iteration count is rejected",
        "argv": ["pagerank", "--iterations", "x", "tests/fixtures/graph/star.tsv"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a damping factor above one is rejected",
        "argv": ["pagerank", "--damping", "2", "tests/fixtures/graph/star.tsv"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "pagerank with no file is rejected",
        "argv": ["pagerank"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
