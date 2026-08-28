"""`search crawl` -- breadth-first order, deduplication, and the limits that end a crawl."""

NAME = "The URL frontier"
ORDER = 350

CASES = [
    {
        "name": "a full crawl visits every reachable allowed page, breadth first",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "http://example.com/"],
        "stdout": "0 200 http://example.com/\n1 200 http://example.com/a.html\n1 200 http://example.com/b.html\n1 200 http://example.com/deep/c.html\n1 200 http://example.com/private/public.html",
    },
    {
        "name": "the page limit stops the crawl mid-level",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--max-pages", "2", "http://example.com/"],
        "stdout": "0 200 http://example.com/\n1 200 http://example.com/a.html",
    },
    {
        "name": "a page limit of one fetches only the seed",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--max-pages", "1", "http://example.com/"],
        "stdout": "0 200 http://example.com/",
    },
    {
        "name": "a page limit of zero fetches nothing",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--max-pages", "0", "http://example.com/"],
        "stdout": "",
    },
    {
        "name": "depth zero follows no links",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--max-depth", "0", "http://example.com/"],
        "stdout": "0 200 http://example.com/",
    },
    {
        "name": "depth one stops before the pages found on page A",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--max-depth", "1", "http://example.com/"],
        "stdout": "0 200 http://example.com/\n1 200 http://example.com/a.html\n1 200 http://example.com/b.html\n1 200 http://example.com/deep/c.html\n1 200 http://example.com/private/public.html",
    },
    {
        "name": "links off the seed host are skipped by default",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--max-depth", "1", "http://example.com/"],
        "stdout": "0 200 http://example.com/\n1 200 http://example.com/a.html\n1 200 http://example.com/b.html\n1 200 http://example.com/deep/c.html\n1 200 http://example.com/private/public.html",
    },
    {
        "name": "all-hosts follows them",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--all-hosts", "--max-depth", "1", "http://example.com/"],
        "stdout": "0 200 http://example.com/\n1 200 http://example.com/a.html\n1 200 http://example.com/b.html\n1 200 http://example.com/deep/c.html\n1 200 http://example.com/private/public.html\n1 200 http://other.example/away.html",
    },
    {
        "name": "a disallowed page is never fetched, even though it is linked",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "http://example.com/"],
        "stdout": "0 200 http://example.com/\n1 200 http://example.com/a.html\n1 200 http://example.com/b.html\n1 200 http://example.com/deep/c.html\n1 200 http://example.com/private/public.html",
    },
    {
        "name": "a page whose bytes were already fetched is dropped",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "http://example.com/"],
        "stdout": "0 200 http://example.com/\n1 200 http://example.com/a.html\n1 200 http://example.com/b.html\n1 200 http://example.com/deep/c.html\n1 200 http://example.com/private/public.html",
    },
    {
        "name": "but it is fetched when reached first",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "http://example.com/copy.html"],
        "stdout": "0 200 http://example.com/copy.html\n1 200 http://example.com/a.html\n1 200 http://example.com/b.html\n1 200 http://example.com/deep/c.html\n1 200 http://example.com/private/public.html",
    },
    {
        "name": "an agent the site bans crawls nothing at all",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--user-agent", "greedybot", "http://example.com/"],
        "stdout": "",
    },
    {
        "name": "a host with no robots.txt permits everything",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "http://norobots.example/"],
        "stdout": "0 200 http://norobots.example/",
    },
    {
        "name": "starting deeper in the site still works",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "http://example.com/deep/c.html"],
        "stdout": "0 200 http://example.com/deep/c.html\n1 200 http://example.com/a.html\n2 200 http://example.com/index.html\n3 200 http://example.com/b.html\n3 200 http://example.com/private/public.html",
    },
    {
        "name": "a missing seed page is reported as a 404 rather than a crash",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "http://example.com/nowhere.html"],
        "stdout": "0 404 http://example.com/nowhere.html",
    },
    {
        "name": "an explicit delay does not change what is fetched",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--delay", "1", "--max-pages", "3", "http://example.com/"],
        "stdout": "0 200 http://example.com/\n1 200 http://example.com/a.html\n1 200 http://example.com/b.html",
    },
    {
        "name": "a bad seed url is rejected",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "notaurl"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing mirror directory is reported",
        "argv": ["crawl", "--mirror", "tests/fixtures/nope", "http://example.com/"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unknown option is rejected",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--nonsense", "1", "http://example.com/"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a non-numeric page limit is rejected",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--max-pages", "x", "http://example.com/"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a non-numeric delay is rejected",
        "argv": ["crawl", "--mirror", "tests/fixtures/site", "--delay", "x", "http://example.com/"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "crawl with no seed is rejected",
        "argv": ["crawl", "--mirror", "tests/fixtures/site"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
