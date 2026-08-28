"""`search wiki-import` -- turning a CirrusSearch dump into a corpus."""

NAME = "Wikipedia import"
ORDER = 480

CASES = [
    {
        "name": "importing reports lines seen, documents kept and entries skipped",
        "argv": ["wiki-import", "tests/fixtures/wiki/sample-cirrus.json", "build/test-wiki.corpus"],
        "stdout": "lines 12\ndocuments 5\nskipped_empty 1",
    },
    {
        "name": "every kept record became a document",
        "argv": ["docs", "build/test-wiki.corpus"],
        "stdout": "doc_00000001\ndoc_00000002\ndoc_00000003\ndoc_00000004\ndoc_00000005",
    },
    {
        "name": "a plain record round trips",
        "argv": ["show", "build/test-wiki.corpus", "doc_00000001"],
        "stdout": "id: doc_00000001\ntitle: Cat\n\nThe cat is a small furry animal. Cats are often kept as pets and they hunt mice.",
    },
    {
        "name": "an escaped quote and apostrophe survive",
        "argv": ["show", "build/test-wiki.corpus", "doc_00000002"],
        "stdout": "id: doc_00000002\ntitle: Dog\n\nThe dog is a domesticated animal. It is often called man's best friend, and it \"barks\".",
    },
    {
        "name": "a unicode escape becomes UTF-8",
        "argv": ["show", "build/test-wiki.corpus", "doc_00000003"],
        "stdout": "id: doc_00000003\ntitle: Café\n\nA café is a place that sells coffee and light meals. The word comes from French.",
    },
    {
        "name": "newlines and tabs inside the text are collapsed",
        "argv": ["show", "build/test-wiki.corpus", "doc_00000004"],
        "stdout": "id: doc_00000004\ntitle: Sun\n\nThe Sun is the star at the centre of the Solar System. It gives light and heat to the planets. It is mostly hydrogen.",
    },
    {
        "name": "a surrogate pair becomes one character, and slashes unescape",
        "argv": ["show", "build/test-wiki.corpus", "doc_00000005"],
        "stdout": "id: doc_00000005\ntitle: Emoji 😀\n\nA surrogate pair encodes an emoji 😀 outside the basic plane. Path C:\\\\Windows and a slash / too.",
    },
    {
        "name": "the corpus indexes like any other",
        "argv": ["index-stats", "build/test-wiki.corpus"],
        "stdout": "documents: 5\nterms: 46\npostings: 49",
    },
    {
        "name": "and answers queries",
        "argv": ["query", "build/test-wiki.corpus", "coffee OR cats"],
        "stdout": "1. doc_00000001  2.1868\n2. doc_00000003  1.4498",
    },
    {
        "name": "and phrases",
        "argv": ["phrase", "build/test-wiki.corpus", "solar system"],
        "stdout": "doc_00000004",
    },
    {
        "name": "and snippets, since the container keeps the text",
        "argv": ["snippet", "--max-chars", "60", "build/test-wiki.corpus", "doc_00000004", "star"],
        "stdout": "Sun\n\nThe Sun is the [star] at the centre of the Solar System....",
    },
    {
        "name": "--abstracts takes opening_text, not text",
        "argv": ["wiki-import", "--abstracts", "tests/fixtures/wiki/sample-cirrus.json", "build/test-wiki-abs.corpus"],
        "stdout": "lines 12\ndocuments 5\nskipped_empty 1",
    },
    {
        "name": "which is the shorter field",
        "argv": ["show", "build/test-wiki-abs.corpus", "doc_00000001"],
        "stdout": "id: doc_00000001\ntitle: Cat\n\nThe cat is a small furry animal.",
    },
    {
        "name": "a limit stops the import early",
        "argv": ["wiki-import", "--limit", "2", "tests/fixtures/wiki/sample-cirrus.json", "build/test-wiki-small.corpus"],
        "stdout": "lines 4\ndocuments 2\nskipped_empty 0",
    },
    {
        "name": "and the corpus is that small",
        "argv": ["docs", "build/test-wiki-small.corpus"],
        "stdout": "doc_00000001\ndoc_00000002",
    },
    {
        "name": "a limit of zero means no limit",
        "argv": ["wiki-import", "--limit", "0", "tests/fixtures/wiki/sample-cirrus.json", "build/test-wiki-all.corpus"],
        "stdout": "lines 12\ndocuments 5\nskipped_empty 1",
    },
    {
        "name": "so everything is imported",
        "argv": ["index-stats", "build/test-wiki-all.corpus"],
        "stdout": "documents: 5\nterms: 46\npostings: 49",
    },
    {
        "name": "the dump can be piped in on stdin",
        "argv": ["wiki-import", "-", "build/test-wiki-stdin.corpus"],
        "stdin": "{\"index\":{\"_type\":\"_doc\",\"_id\":\"1\"}}\n{\"template\":[\"Template:Infobox\"],\"content_model\":\"wikitext\",\"opening_text\":\"The cat is a small furry animal.\",\"wiki\":\"simplewiki\",\"language\":\"en\",\"title\":\"Cat\",\"text\":\"The cat is a small furry animal. Cats are often kept as pets and they hunt mice.\",\"category\":[\"Mammals\"]}\n{\"index\":{\"_type\":\"_doc\",\"_id\":\"2\"}}\n{\"content_model\":\"wikitext\",\"opening_text\":\"The dog is a domesticated animal.\",\"wiki\":\"simplewiki\",\"language\":\"en\",\"title\":\"Dog\",\"text\":\"The dog is a domesticated animal. It is often called man's best friend, and it \\\"barks\\\".\",\"auxiliary_text\":[\"See also\"]}\n{\"index\":{\"_type\":\"_doc\",\"_id\":\"3\"}}\n{\"content_model\":\"wikitext\",\"opening_text\":\"\",\"wiki\":\"simplewiki\",\"language\":\"en\",\"title\":\"Empty Article\",\"text\":\"\",\"category\":[]}\n{\"index\":{\"_type\":\"_doc\",\"_id\":\"4\"}}\n{\"content_model\":\"wikitext\",\"opening_text\":\"A caf\\u00e9 sells coffee.\",\"wiki\":\"simplewiki\",\"language\":\"en\",\"title\":\"Caf\\u00e9\",\"text\":\"A caf\\u00e9 is a place that sells coffee and light meals. The word comes from French.\",\"category\":[\"Food\"]}\n{\"index\":{\"_type\":\"_doc\",\"_id\":\"5\"}}\n{\"content_model\":\"wikitext\",\"opening_text\":\"The Sun is a star.\",\"wiki\":\"simplewiki\",\"language\":\"en\",\"title\":\"Sun\",\"text\":\"The Sun is the star at the centre of the Solar System.\\nIt gives light and heat to the planets.\\tIt is mostly hydrogen.\",\"category\":[\"Astronomy\"]}\n{\"index\":{\"_type\":\"_doc\",\"_id\":\"6\"}}\n{\"content_model\":\"wikitext\",\"opening_text\":\"An emoji test.\",\"wiki\":\"simplewiki\",\"language\":\"en\",\"title\":\"Emoji \\ud83d\\ude00\",\"text\":\"A surrogate pair encodes an emoji \\ud83d\\ude00 outside the basic plane. Path C:\\\\\\\\Windows and a slash \\/ too.\",\"category\":[]}\n",
        "stdout": "lines 12\ndocuments 5\nskipped_empty 1",
    },
    {
        "name": "and produces the same documents",
        "argv": ["docs", "build/test-wiki-stdin.corpus"],
        "stdout": "doc_00000001\ndoc_00000002\ndoc_00000003\ndoc_00000004\ndoc_00000005",
    },
    {
        "name": "a missing dump is reported",
        "argv": ["wiki-import", "tests/fixtures/nope.json", "build/test-wiki.corpus"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unwritable destination is reported",
        "argv": ["wiki-import", "tests/fixtures/wiki/sample-cirrus.json", "/nope/x.corpus"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a non-numeric limit is rejected",
        "argv": ["wiki-import", "--limit", "x", "tests/fixtures/wiki/sample-cirrus.json", "build/test-wiki.corpus"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unknown option is rejected",
        "argv": ["wiki-import", "--bogus", "tests/fixtures/wiki/sample-cirrus.json", "build/test-wiki.corpus"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "wiki-import with no destination is rejected",
        "argv": ["wiki-import", "tests/fixtures/wiki/sample-cirrus.json"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
    {
        "name": "a dump written with spaces around its colons still imports",
        "argv": ["wiki-import", "tests/fixtures/wiki/sample-spaced.json", "build/test-wiki-spaced.corpus"],
        "stdout": "lines 2\ndocuments 1\nskipped_empty 0",
    },
    {
        "name": "and the fields were read from the right keys",
        "argv": ["show", "build/test-wiki-spaced.corpus", "doc_00000001"],
        "stdout": "id: doc_00000001\ntitle: Spaced Record\n\nA record written with spaces around its colons and commas.",
    },
]
