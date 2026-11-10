"""`search serve` -- the browser front end, driven one request at a time on stdin."""

NAME = "HTTP front end"
ORDER = 550


def response(status, reason, content_type, body):
    return (
        f"HTTP/1.1 {status} {reason}\n"
        f"Content-Type: {content_type}\n"
        f"Content-Length: {len(body.encode())}\n"
        "Connection: close\n"
        "\n"
        f"{body}"
    )


def request(target, method="GET"):
    return f"{method} {target} HTTP/1.1\r\nHost: localhost\r\n\r\n"


JSON = "application/json"
TEXT = "text/plain; charset=utf-8"

PAGE_C = ('{"id":"doc_0004","score":%s,"title":"Page C","url":"http://example.com/deep/c.html",'
          '"snippet":"[Page] C [Page] C The deepest [page] mentions a planet. up to A"}')

CASES = [
    {
        "name": "the JSON API ranks, titles, links and excerpts each result",
        "argv": ["serve", "--stdin", "build/crawled"],
        "stdin": request("/api/search?q=page&limit=2"),
        "stdout": response(200, "OK", JSON,
                           '{"query":"page","suggestion":null,"results":[' + PAGE_C % "0.4881" +
                           ',{"id":"doc_0003","score":0.4271,"title":"Page B",'
                           '"url":"http://example.com/b.html",'
                           '"snippet":"[Page] B [Page] B Recipes for the oven &mdash; bread & cake."}]}\n'),
    },
    {
        "name": "a pagerank weight in the url reranks by link authority",
        "argv": ["serve", "--stdin", "build/crawled"],
        "stdin": request("/api/search?q=page&limit=1&pagerank=1"),
        "stdout": response(200, "OK", JSON,
                           '{"query":"page","suggestion":null,"results":[' + PAGE_C % "1.0344" +
                           ']}\n'),
    },
    {
        "name": "a saved index finds its urls in the file index-write left beside it",
        "argv": ["serve", "--stdin", "build/crawl-prior.idx"],
        "stdin": request("/api/search?q=page&limit=1&pagerank=1"),
        "stdout_contains": ['"id":"doc_0004","score":1.0344', '"url":"http://example.com/deep/c.html"'],
    },
    {
        "name": "a misspelled query comes back with a suggestion",
        "argv": ["serve", "--stdin", "build/crawled"],
        "stdin": request("/api/search?q=plnet"),
        "stdout": response(200, "OK", JSON,
                           '{"query":"plnet","suggestion":"planet","results":[]}\n'),
    },
    {
        "name": "percent-encoded queries are decoded, and quotes in them escaped",
        "argv": ["serve", "--stdin", "build/crawled"],
        "stdin": request("/api/search?q=%22deepest+page%22"),
        "stdout": response(200, "OK", JSON,
                           '{"query":"\\"deepest page\\"","suggestion":null,"results":['
                           '{"id":"doc_0004","score":2.1016,"title":"Page C",'
                           '"url":"http://example.com/deep/c.html",'
                           '"snippet":"[Page] C [Page] C The [deepest] [page] mentions a planet. up to A"}]}\n'),
    },
    {
        "name": "a query that does not parse is a 400 with the reason",
        "argv": ["serve", "--stdin", "build/crawled"],
        "stdin": request("/api/search?q=%22open"),
        "stdout": response(400, "Bad Request", JSON,
                           '{"query":"\\"open","error":"unterminated quote"}\n'),
    },
    {
        "name": "so is a limit that is not a small whole number",
        "argv": ["serve", "--stdin", "build/crawled"],
        "stdin": request("/api/search?q=page&limit=0"),
        "stdout": response(400, "Bad Request", JSON,
                           '{"query":"page","error":"limit must be a whole number from 1 to 100"}\n'),
    },
    {
        "name": "user text is escaped in the HTML page",
        "argv": ["serve", "--stdin", "build/crawled"],
        "stdin": request("/search?q=%3Cscript%3Ealert(1)"),
        "stdout_contains": ["<title>&lt;script&gt;alert(1) - Search</title>",
                            'value="&lt;script&gt;alert(1)"',
                            '<p class="note">No documents match.</p>'],
        "stdout_lacks": ["<script>alert"],
    },
    {
        "name": "completions come from the indexed terms, most common first",
        "argv": ["serve", "--stdin", "corpus/fixtures"],
        "stdin": request("/api/complete?q=pla"),
        "stdout": response(200, "OK", JSON,
                           '{"prefix":"pla","completions":[{"term":"planet","documents":3},'
                           '{"term":"plan","documents":1}]}\n'),
    },
    {
        "name": "the prefix is normalized the way indexed terms are",
        "argv": ["serve", "--stdin", "corpus/fixtures"],
        "stdin": request("/api/complete?q=PLA&limit=1"),
        "stdout": response(200, "OK", JSON,
                           '{"prefix":"pla","completions":[{"term":"planet","documents":3}]}\n'),
    },
    {
        "name": "an empty prefix completes to nothing rather than the whole dictionary",
        "argv": ["serve", "--stdin", "corpus/fixtures"],
        "stdin": request("/api/complete?q="),
        "stdout": response(200, "OK", JSON, '{"prefix":"","completions":[]}\n'),
    },
    {
        "name": "a bad completion limit is a 400",
        "argv": ["serve", "--stdin", "corpus/fixtures"],
        "stdin": request("/api/complete?q=pla&limit=0"),
        "stdout": response(400, "Bad Request", JSON,
                           '{"error":"limit must be a whole number from 1 to 100"}\n'),
    },
    {
        "name": "the search page wires the box to the completion list",
        "argv": ["serve", "--stdin", "corpus/fixtures"],
        "stdin": request("/"),
        "stdout_contains": ['role="combobox"', 'aria-controls="suggest"',
                            '<ul id="suggest" role="listbox" hidden></ul>', "/api/complete?limit=8&q="],
    },
    {
        "name": "an unknown path is a 404",
        "argv": ["serve", "--stdin", "build/crawled"],
        "stdin": request("/admin"),
        "stdout": response(404, "Not Found", TEXT, "not found\n"),
    },
    {
        "name": "anything but GET is refused",
        "argv": ["serve", "--stdin", "build/crawled"],
        "stdin": request("/", method="POST"),
        "stdout": response(405, "Method Not Allowed", TEXT, "only GET is supported\n"),
    },
    {
        "name": "a request that is not HTTP is a 400",
        "argv": ["serve", "--stdin", "build/crawled"],
        "stdin": "hello\r\n\r\n",
        "stdout": response(400, "Bad Request", TEXT, "bad request\n"),
    },
    {
        "name": "a bad port is rejected",
        "argv": ["serve", "--port", "70000", "build/crawled"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "serve with no source is rejected",
        "argv": ["serve"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
