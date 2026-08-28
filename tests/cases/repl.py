"""`search repl <source>` -- reading queries until end of input."""

NAME = "The REPL"
ORDER = 380

CASES = [
    {
        "name": "a query typed at the prompt is answered",
        "argv": ["repl", "corpus/fixtures"],
        "stdin": "cat AND mat\n",
        "stdout": "1. doc_001  7.4906",
    },
    {
        "name": "several queries in one session",
        "argv": ["repl", "tests/fixtures/tiny"],
        "stdin": "cat\ndog\n",
        "stdout": "1. alpha  0.4789\n2. gamma  0.4532\n1. beta  0.4789\n2. gamma  0.4532",
    },
    {
        "name": "blank lines are ignored",
        "argv": ["repl", "tests/fixtures/tiny"],
        "stdin": "\n   \ncat\n",
        "stdout": "1. alpha  0.4789\n2. gamma  0.4532",
    },
    {
        "name": "leading and trailing spaces are trimmed",
        "argv": ["repl", "tests/fixtures/tiny"],
        "stdin": "   cat AND dog   \n",
        "stdout": "1. gamma  0.9063",
    },
    {
        "name": "the limit can be changed mid-session",
        "argv": ["repl", "corpus/fixtures"],
        "stdin": ":limit 2\ncat\n",
        "stdout": "1. doc_003  2.5076\n2. doc_001  2.3900",
    },
    {
        "name": "so can the scorer",
        "argv": ["repl", "corpus/fixtures"],
        "stdin": ":limit 2\n:scorer tfidf\ncat\n",
        "stdout": "1. doc_003  3.7975\n2. doc_001  3.4727",
    },
    {
        "name": "settings can be inspected",
        "argv": ["repl", "corpus/fixtures"],
        "stdin": ":settings\n",
        "stdout": "limit 10\nscorer bm25\nsnippet off",
    },
    {
        "name": "settings reflect what was changed",
        "argv": ["repl", "corpus/fixtures"],
        "stdin": ":limit 3\n:scorer tfidf\n:snippet on\n:settings\n",
        "stdout": "limit 3\nscorer tfidf\nsnippet on",
    },
    {
        "name": "snippets can be switched on",
        "argv": ["repl", "corpus/fixtures"],
        "stdin": ":snippet on\n:limit 1\nplanet\n",
        "stdout": "1. doc_026  2.3390\n   The Sun The sun is a star at the centre of our solar system. The sun burns hydrogen and gives light and heat to the [planets].",
    },
    {
        "name": "help lists the commands",
        "argv": ["repl", "corpus/fixtures"],
        "stdin": ":help\n",
        "stdout": "queries use AND, OR, NOT, parentheses and \"phrases\"\n:limit <n>      how many results to show\n:scorer <name>  bm25 or tfidf\n:snippet <on|off>\n:settings       show the current settings\n:quit",
    },
    {
        "name": "quit ends the session early",
        "argv": ["repl", "tests/fixtures/tiny"],
        "stdin": "cat\n:quit\ndog\n",
        "stdout": "1. alpha  0.4789\n2. gamma  0.4532",
    },
    {
        "name": "the short form of quit works too",
        "argv": ["repl", "tests/fixtures/tiny"],
        "stdin": ":q\ncat\n",
        "stdout": "",
    },
    {
        "name": "an unknown meta command says so and continues",
        "argv": ["repl", "tests/fixtures/tiny"],
        "stdin": ":bogus\ncat\n",
        "stdout": "unknown command: :bogus\n1. alpha  0.4789\n2. gamma  0.4532",
    },
    {
        "name": "a malformed query is reported and the session continues",
        "argv": ["repl", "tests/fixtures/tiny"],
        "stdin": "(cat\ncat\n",
        "stdout": "error: missing closing parenthesis\n1. alpha  0.4789\n2. gamma  0.4532",
    },
    {
        "name": "a bad setting value is rejected without changing anything",
        "argv": ["repl", "corpus/fixtures"],
        "stdin": ":limit x\n:settings\n",
        "stdout": "limit must be a non-negative integer\nlimit 10\nscorer bm25\nsnippet off",
    },
    {
        "name": "an unknown scorer is rejected",
        "argv": ["repl", "corpus/fixtures"],
        "stdin": ":scorer bogus\n:settings\n",
        "stdout": "scorer must be bm25 or tfidf\nlimit 10\nscorer bm25\nsnippet off",
    },
    {
        "name": "end of input ends the session",
        "argv": ["repl", "tests/fixtures/tiny"],
        "stdin": "cat",
        "stdout": "1. alpha  0.4789\n2. gamma  0.4532",
    },
    {
        "name": "no input at all is a clean exit",
        "argv": ["repl", "tests/fixtures/tiny"],
        "stdout": "",
    },
    {
        "name": "a saved index works, without snippets",
        "argv": ["repl", "tests/fixtures/index/good.bin"],
        "stdin": ":snippet on\ncat\n",
        "stdout": "1. alpha  0.4789\n2. gamma  0.4532",
    },
    {
        "name": "flags set the starting options",
        "argv": ["repl", "--limit", "1", "--scorer", "tfidf", "corpus/fixtures"],
        "stdin": "cat\n",
        "stdout": "1. doc_003  3.7975",
    },
    {
        "name": "a missing source is reported",
        "argv": ["repl", "tests/fixtures/nope"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unknown flag is rejected",
        "argv": ["repl", "--bogus", "1", "corpus/fixtures"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "repl with no source is rejected",
        "argv": ["repl"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
