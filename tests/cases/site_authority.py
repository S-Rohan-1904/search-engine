"""`--pagerank-graph site|home` -- PageRank over hosts, or a flat home-page prior."""

NAME = "Site-level link authority"
ORDER = 545

SITES = "tests/fixtures/sites"

PLAIN = (
    "doc_0002\t0.1089\ndoc_0004\t0.0987\ndoc_0001\t0.0672\ndoc_0003\t0.0672\n"
    "doc_0007\t0.0672\ndoc_0005\t0.0641\ndoc_0006\t0.0613"
)
PAGE_BOOSTED = (
    "doc_0002\t0.1725\ndoc_0004\t0.1563\ndoc_0007\t0.1391\ndoc_0001\t0.1198\n"
    "doc_0003\t0.1198\ndoc_0006\t0.0875\ndoc_0005\t0.0803"
)
SITE_BOOSTED = (
    "doc_0003\t0.1091\ndoc_0002\t0.1089\ndoc_0001\t0.1015\ndoc_0004\t0.0987\n"
    "doc_0007\t0.0672\ndoc_0005\t0.0641\ndoc_0006\t0.0613"
)

HOME_BOOSTED = (
    "doc_0001\t0.1138\ndoc_0003\t0.1138\ndoc_0007\t0.1138\ndoc_0002\t0.1089\n"
    "doc_0004\t0.0987\ndoc_0005\t0.0641\ndoc_0006\t0.0613"
)



def request(target):
    return f"GET {target} HTTP/1.1\r\nHost: localhost\r\n\r\n"


CASES = [
    {
        "name": "links within a host and to uncrawled pages are dropped; scores go to home pages",
        "argv": ["pagerank", "--sites", f"{SITES}/urls.tsv", f"{SITES}/links.tsv"],
        "stdout": "doc_0003 0.432749\ndoc_0001 0.333333",
    },
    {
        "name": "without the flag, PageRank is over pages",
        "argv": ["query", "--tsv", "--pagerank", "1", SITES, "garden"],
        "stdout": PAGE_BOOSTED,
    },
    {
        "name": "which is what naming the page graph gives",
        "argv": ["query", "--tsv", "--pagerank", "1", "--pagerank-graph", "page", SITES, "garden"],
        "stdout": PAGE_BOOSTED,
    },
    {
        "name": "the site graph lifts only the home pages of linked-to sites",
        "argv": ["query", "--tsv", "--pagerank", "1", "--pagerank-graph", "site", SITES, "garden"],
        "stdout": SITE_BOOSTED,
    },
    {
        "name": "a weight of zero leaves the ranking plain whichever graph is chosen",
        "argv": ["query", "--tsv", "--pagerank", "0", "--pagerank-graph", "site", SITES, "garden"],
        "stdout": PLAIN,
    },
    {
        "name": "index-write keeps the site ranking beside the index",
        "argv": ["index-write", SITES, "build/sites-prior.idx"],
        "stdout": "",
    },
    {
        "name": "and the index ranks exactly as the directory did",
        "argv": ["query", "--tsv", "--pagerank", "1", "--pagerank-graph", "site",
                 "build/sites-prior.idx", "garden"],
        "stdout": SITE_BOOSTED,
    },
    {
        "name": "the page ranking is still kept beside it",
        "argv": ["query", "--tsv", "--pagerank", "1", "build/sites-prior.idx", "garden"],
        "stdout": PAGE_BOOSTED,
    },
    {
        "name": "pagerank --sites --out writes the file index-write keeps",
        "argv": ["pagerank", "--sites", f"{SITES}/urls.tsv", "--out",
                 "build/sites-prior.idx.siterank", f"{SITES}/links.tsv"],
        "stdout": "",
    },
    {
        "name": "and an index ranks with it exactly as before",
        "argv": ["query", "--tsv", "--pagerank", "1", "--pagerank-graph", "site",
                 "build/sites-prior.idx", "garden"],
        "stdout": SITE_BOOSTED,
    },
    {
        "name": "the REPL takes the graph as an option",
        "argv": ["repl", "--tsv", "--pagerank-graph", "site", SITES],
        "stdin": ":pagerank 1\ngarden\n:pagerank 0\ngarden\n",
        "stdout": SITE_BOOSTED + "\n" + PLAIN,
    },
    {
        "name": "the home graph lifts every home page alike, linked to or not",
        "argv": ["query", "--tsv", "--pagerank", "1", "--pagerank-graph", "home", SITES, "garden"],
        "stdout": HOME_BOOSTED,
    },
    {
        "name": "and an index finds its home pages in the urls kept beside it",
        "argv": ["query", "--tsv", "--pagerank", "1", "--pagerank-graph", "home",
                 "build/sites-prior.idx", "garden"],
        "stdout": HOME_BOOSTED,
    },
    {
        "name": "an index with no urls beside it has no home pages to lift",
        "argv": ["query", "--pagerank", "1", "--pagerank-graph", "home",
                 "build/fx-no-links.idx", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "the REPL switches graph between queries",
        "argv": ["repl", "--tsv", "--pagerank", "1", SITES],
        "stdin": "garden\n:pagerank-graph site\ngarden\n:pagerank-graph home\ngarden\n"
                 ":settings\n",
        "stdout": PAGE_BOOSTED + "\n" + SITE_BOOSTED + "\n" + HOME_BOOSTED +
                  "\nlimit 10\nscorer bm25\nsnippet off\npagerank 1\npagerank-graph home"
                  "\npagerank-max-words 0",
    },
    {
        "name": "the REPL refuses a graph it has no ranking for and keeps the old one",
        "argv": ["repl", "--tsv", "build/fx-no-links.idx"],
        "stdin": ":pagerank-graph site\n:pagerank-graph tree\n:settings\n",
        "stdout": "no site ranking for this source\npagerank-graph must be page, site or home\n"
                  "limit 10\nscorer bm25\nsnippet off\npagerank 0\npagerank-graph page"
                  "\npagerank-max-words 0",
    },
    {
        "name": "the server takes the graph from the url",
        "argv": ["serve", "--stdin", SITES],
        "stdin": request("/api/search?q=garden&limit=1&pagerank=1&graph=site"),
        "stdout_contains": ['"results":[{"id":"doc_0003","score":0.1091,'],
    },
    {
        "name": "and the home graph",
        "argv": ["serve", "--stdin", SITES],
        "stdin": request("/api/search?q=garden&limit=1&pagerank=1&graph=home"),
        "stdout_contains": ['"results":[{"id":"doc_0001","score":0.1138,'],
    },
    {
        "name": "an index serves the graphs kept beside it",
        "argv": ["serve", "--stdin", "build/sites-prior.idx"],
        "stdin": request("/api/search?q=garden&limit=1&pagerank=1&graph=site"),
        "stdout_contains": ['"results":[{"id":"doc_0003","score":0.1091,'],
    },
    {
        "name": "an unknown graph in the url is a 400",
        "argv": ["serve", "--stdin", SITES],
        "stdin": request("/api/search?q=garden&pagerank=1&graph=tree"),
        "stdout_contains": ["HTTP/1.1 400", "graph must be page, site or home"],
    },
    {
        "name": "so is asking for link authority the index has no ranking for",
        "argv": ["serve", "--stdin", "build/fx-no-links.idx"],
        "stdin": request("/api/search?q=cat&pagerank=1&graph=site"),
        "stdout_contains": ["HTTP/1.1 400", "no site ranking for this index"],
    },
    {
        "name": "an index with no site ranking beside it is an error",
        "argv": ["query", "--pagerank", "1", "--pagerank-graph", "site",
                 "build/fx-no-links.idx", "cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unknown graph is rejected",
        "argv": ["query", "--pagerank", "1", "--pagerank-graph", "host", SITES, "garden"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing urls file is an error",
        "argv": ["pagerank", "--sites", f"{SITES}/missing.tsv", f"{SITES}/links.tsv"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
]
