"""`search url`, `url-resolve`, `robots` and `crawl-delay` -- addressing, permission and politeness."""

NAME = "URLs and robots.txt"
ORDER = 340

CASES = [
    {
        "name": "scheme and host are lowercased and an empty path becomes a slash",
        "argv": ["url", "HTTP://Example.COM"],
        "stdout": "http://example.com/",
    },
    {
        "name": "a default port is dropped",
        "argv": ["url", "http://example.com:80/a"],
        "stdout": "http://example.com/a",
    },
    {
        "name": "a non-default port is kept",
        "argv": ["url", "http://example.com:8080/a"],
        "stdout": "http://example.com:8080/a",
    },
    {
        "name": "dot segments are resolved",
        "argv": ["url", "http://example.com/a/../b/./c"],
        "stdout": "http://example.com/b/c",
    },
    {
        "name": "a fragment names a place in a page, not a page, so it goes",
        "argv": ["url", "https://x.io:443/p?q=1#frag"],
        "stdout": "https://x.io/p?q=1",
    },
    {
        "name": "a query is kept",
        "argv": ["url", "http://e.com/s?q=cats&p=2"],
        "stdout": "http://e.com/s?q=cats&p=2",
    },
    {
        "name": "a relative link resolves against the containing directory",
        "argv": ["url-resolve", "http://example.com/deep/c.html", "a.html"],
        "stdout": "http://example.com/deep/a.html",
    },
    {
        "name": "a root-relative link ignores the directory",
        "argv": ["url-resolve", "http://example.com/deep/c.html", "/b.html"],
        "stdout": "http://example.com/b.html",
    },
    {
        "name": "a parent reference climbs one level",
        "argv": ["url-resolve", "http://example.com/deep/c.html", "../up.html"],
        "stdout": "http://example.com/up.html",
    },
    {
        "name": "a protocol-relative link inherits the scheme",
        "argv": ["url-resolve", "https://example.com/a", "//cdn.example/x"],
        "stdout": "https://cdn.example/x",
    },
    {
        "name": "an absolute link is taken as it is",
        "argv": ["url-resolve", "http://example.com/a", "http://z.io/y"],
        "stdout": "http://z.io/y",
    },
    {
        "name": "whitespace around a link is ignored",
        "argv": ["url-resolve", "http://example.com/a", "  b.html  "],
        "stdout": "http://example.com/b.html",
    },
    {
        "name": "an ordinary path is allowed",
        "argv": ["robots", "tests/fixtures/site/example.com/robots.txt", "searchbot", "/index.html"],
        "stdout": "allow",
    },
    {
        "name": "a disallowed directory is refused",
        "argv": ["robots", "tests/fixtures/site/example.com/robots.txt", "searchbot", "/private/secret.html"],
        "stdout": "disallow",
    },
    {
        "name": "an Allow rule beats a shorter Disallow",
        "argv": ["robots", "tests/fixtures/site/example.com/robots.txt", "searchbot", "/private/public.html"],
        "stdout": "allow",
    },
    {
        "name": "a wildcard with an end anchor matches",
        "argv": ["robots", "tests/fixtures/site/example.com/robots.txt", "searchbot", "/manual.pdf"],
        "stdout": "disallow",
    },
    {
        "name": "the anchor means the extension must end the path",
        "argv": ["robots", "tests/fixtures/site/example.com/robots.txt", "searchbot", "/a.pdf.html"],
        "stdout": "allow",
    },
    {
        "name": "a named group overrides the wildcard group entirely",
        "argv": ["robots", "tests/fixtures/site/example.com/robots.txt", "greedybot", "/index.html"],
        "stdout": "disallow",
    },
    {
        "name": "agent matching is by prefix and case-insensitive",
        "argv": ["robots", "tests/fixtures/site/example.com/robots.txt", "GreedyBot/2.0", "/index.html"],
        "stdout": "disallow",
    },
    {
        "name": "an unlisted agent falls back to the wildcard group",
        "argv": ["robots", "tests/fixtures/site/example.com/robots.txt", "otherbot", "/private/secret.html"],
        "stdout": "disallow",
    },
    {
        "name": "a group with a Crawl-delay still applies its rules",
        "argv": ["robots", "tests/fixtures/site/example.com/robots.txt", "slowbot", "/private/secret.html"],
        "stdout": "disallow",
    },
    {
        "name": "with no Crawl-delay the floor is used",
        "argv": ["crawl-delay", "tests/fixtures/site/example.com/robots.txt", "searchbot", "500"],
        "stdout": "500",
    },
    {
        "name": "a site asking for longer than the floor gets it",
        "argv": ["crawl-delay", "tests/fixtures/site/example.com/robots.txt", "slowbot", "500"],
        "stdout": "2500",
    },
    {
        "name": "a site asking for less than the floor does not get it",
        "argv": ["crawl-delay", "tests/fixtures/site/example.com/robots.txt", "slowbot", "5000"],
        "stdout": "5000",
    },
    {
        "name": "fractional seconds become milliseconds",
        "argv": ["crawl-delay", "tests/fixtures/site/example.com/robots.txt", "slowbot", "0"],
        "stdout": "2500",
    },
    {
        "name": "a floor of zero with no directive means no delay",
        "argv": ["crawl-delay", "tests/fixtures/site/example.com/robots.txt", "searchbot", "0"],
        "stdout": "0",
    },
    {
        "name": "a non-http scheme is rejected",
        "argv": ["url", "ftp://e.com/"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "text that is not a url is rejected",
        "argv": ["url", "notaurl"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a mailto link does not resolve",
        "argv": ["url-resolve", "http://example.com/a", "mailto:a@b"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a bare fragment does not resolve",
        "argv": ["url-resolve", "http://example.com/a", "#frag"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a missing robots file is reported",
        "argv": ["robots", "tests/fixtures/nope.txt", "bot", "/"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a non-numeric floor is rejected",
        "argv": ["crawl-delay", "tests/fixtures/site/example.com/robots.txt", "searchbot", "x"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "url with no argument is rejected",
        "argv": ["url"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
