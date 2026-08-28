"""`search html-text` and `html-links` -- extracting prose and links from markup."""

NAME = "HTML to text"
ORDER = 360

CASES = [
    {
        "name": "title and visible text, with tags and script content gone",
        "argv": ["html-text", "tests/fixtures/site/example.com/index.html"],
        "stdout": "title: Example Home\nExample Home Welcome & hello The cat sat on the mat. See also page A and page B. deep page secret public manual another host mail top",
    },
    {
        "name": "entities are decoded and unknown ones left alone",
        "argv": ["html-text", "tests/fixtures/site/example.com/b.html"],
        "stdout": "title: Page B\nPage B Recipes for the oven &mdash; bread & cake.",
    },
    {
        "name": "a simple page",
        "argv": ["html-text", "tests/fixtures/site/example.com/a.html"],
        "stdout": "title: Page A\nPage A Cats and dogs live here. Back to home, onward to c, and a byte-identical copy of home.",
    },
    {
        "name": "links come back in document order, resolved against the base",
        "argv": ["html-links", "http://example.com/", "tests/fixtures/site/example.com/index.html"],
        "stdout": "http://example.com/a.html\nhttp://example.com/b.html\nhttp://example.com/deep/c.html\nhttp://example.com/private/secret.html\nhttp://example.com/private/public.html\nhttp://example.com/manual.pdf\nhttp://other.example/away.html",
    },
    {
        "name": "relative links resolve against the page's directory",
        "argv": ["html-links", "http://example.com/deep/c.html", "tests/fixtures/site/example.com/deep/c.html"],
        "stdout": "http://example.com/a.html",
    },
    {
        "name": "a page with no links yields none",
        "argv": ["html-links", "http://example.com/", "tests/fixtures/site/example.com/b.html"],
        "stdout": "",
    },
    {
        "name": "a missing file is reported",
        "argv": ["html-text", "tests/fixtures/nope.html"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a bad base url is reported",
        "argv": ["html-links", "notaurl", "tests/fixtures/site/example.com/a.html"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "html-text with no file is rejected",
        "argv": ["html-text"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
