"""`--pagerank-max-words` -- link authority only for short queries."""

NAME = "Link authority for short queries"
ORDER = 546

SITES = "tests/fixtures/sites"
SITE = ["--pagerank", "1", "--pagerank-graph", "site"]

TWO_PLAIN = (
    "doc_0004\t1.3628\ndoc_0003\t0.9280\ndoc_0005\t0.8854\ndoc_0002\t0.1089\n"
    "doc_0001\t0.0672\ndoc_0007\t0.0672\ndoc_0006\t0.0613"
)
TWO_BOOSTED = (
    "doc_0003\t1.5066\ndoc_0004\t1.3628\ndoc_0005\t0.8854\ndoc_0002\t0.1089\n"
    "doc_0001\t0.1015\ndoc_0007\t0.0672\ndoc_0006\t0.0613"
)
THREE_PLAIN = (
    "doc_0004\t1.3628\ndoc_0001\t1.2783\ndoc_0002\t1.2644\ndoc_0003\t0.9280\ndoc_0005\t0.8854"
)
THREE_BOOSTED = (
    "doc_0001\t1.9313\ndoc_0003\t1.5066\ndoc_0004\t1.3628\ndoc_0002\t1.2644\ndoc_0005\t0.8854"
)
PHRASE_PLAIN = "doc_0001\t1.4795\ndoc_0003\t1.4795\ndoc_0007\t1.4795\ndoc_0005\t1.4233"
PHRASE_BOOSTED = "doc_0003\t2.4021\ndoc_0001\t2.2353\ndoc_0007\t1.4795\ndoc_0005\t1.4233"



def request(target):
    return f"GET {target} HTTP/1.1\r\nHost: localhost\r\n\r\n"


CASES = [
    {
        "name": "without a limit every query gets link authority",
        "argv": ["query", "--tsv", *SITE, SITES, "garden AND (news OR tools)"],
        "stdout": THREE_BOOSTED,
    },
    {
        "name": "a limit of zero is the same as none",
        "argv": ["query", "--tsv", *SITE, "--pagerank-max-words", "0", SITES,
                 "garden AND (news OR tools)"],
        "stdout": THREE_BOOSTED,
    },
    {
        "name": "a two-word query is short enough",
        "argv": ["query", "--tsv", *SITE, "--pagerank-max-words", "2", SITES, "garden OR news"],
        "stdout": TWO_BOOSTED,
    },
    {
        "name": "operators and parentheses are not words, so three terms are too long",
        "argv": ["query", "--tsv", *SITE, "--pagerank-max-words", "2", SITES,
                 "garden AND (news OR tools)"],
        "stdout": THREE_PLAIN,
    },
    {
        "name": "and a higher limit lets them through",
        "argv": ["query", "--tsv", *SITE, "--pagerank-max-words", "3", SITES,
                 "garden AND (news OR tools)"],
        "stdout": THREE_BOOSTED,
    },
    {
        "name": "each word of a quoted phrase counts",
        "argv": ["query", "--tsv", *SITE, "--pagerank-max-words", "2", SITES,
                 '"home page" garden'],
        "stdout": PHRASE_PLAIN,
    },
    {
        "name": "so a phrase and a word need a limit of three",
        "argv": ["query", "--tsv", *SITE, "--pagerank-max-words", "3", SITES,
                 '"home page" garden'],
        "stdout": PHRASE_BOOSTED,
    },
    {
        "name": "the REPL routes each query on its own length",
        "argv": ["repl", "--tsv", *SITE, "--pagerank-max-words", "2", SITES],
        "stdin": "garden OR news\ngarden AND (news OR tools)\n",
        "stdout": TWO_BOOSTED + "\n" + THREE_PLAIN,
    },
    {
        "name": "the limit applies to the page graph as well",
        "argv": ["query", "--tsv", "--pagerank", "1", "--pagerank-max-words", "1", SITES,
                 "garden OR news"],
        "stdout": TWO_PLAIN,
    },
    {
        "name": "the REPL sets the limit between queries",
        "argv": ["repl", "--tsv", *SITE, SITES],
        "stdin": ":pagerank-max-words 2\ngarden AND (news OR tools)\n:pagerank-max-words 0\n"
                 "garden AND (news OR tools)\n:pagerank-max-words some\n:settings\n",
        "stdout": THREE_PLAIN + "\n" + THREE_BOOSTED +
                  "\npagerank-max-words must be a non-negative integer"
                  "\nlimit 10\nscorer bm25\nsnippet off\npagerank 1\npagerank-graph site"
                  "\npagerank-max-words 0",
    },
    {
        "name": "the server takes the limit from the url",
        "argv": ["serve", "--stdin", SITES],
        "stdin": request("/api/search?q=garden+AND+(news+OR+tools)&limit=1&pagerank=1"
                         "&graph=site&max_words=2"),
        "stdout_contains": ['"results":[{"id":"doc_0004","score":1.3628,'],
    },
    {
        "name": "a bad limit in the url is a 400",
        "argv": ["serve", "--stdin", SITES],
        "stdin": request("/api/search?q=garden&pagerank=1&max_words=-1"),
        "stdout_contains": ["HTTP/1.1 400", "max_words must be a whole number"],
    },
    {
        "name": "a limit that is not a whole number is rejected",
        "argv": ["query", *SITE, "--pagerank-max-words", "two", SITES, "garden"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
]
