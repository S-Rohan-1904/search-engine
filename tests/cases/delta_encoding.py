"""Delta encoding stores gaps rather than absolute ids, and must read back identically."""

NAME = "Delta-encoded document ids"
ORDER = 280

CASES = [
    {
        "name": "a delta-encoded index of the working corpus is written",
        "argv": ["index-write", "--encoding", "delta", "corpus/fixtures", "build/delta-index.bin"],
        "stdout": "",
    },
    {
        "name": "its counts match the corpus",
        "argv": ["index-stats", "build/delta-index.bin"],
        "stdout": "documents: 30\nterms: 235\npostings: 343",
    },
    {
        "name": "the corpus reports the same counts",
        "argv": ["index-stats", "corpus/fixtures"],
        "stdout": "documents: 30\nterms: 235\npostings: 343",
    },
    {
        "name": "postings decode back to absolute document ids",
        "argv": ["postings", "build/delta-index.bin", "cat"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "the corpus gives those same postings",
        "argv": ["postings", "corpus/fixtures", "cat"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "positions decode back to absolute positions",
        "argv": ["positions", "build/delta-index.bin", "cat"],
        "stdout": "doc_001 0 4 10 24\ndoc_002 15\ndoc_003 2 5 13 18 24\ndoc_004 2 6 18\ndoc_005 16\ndoc_006 18\ndoc_008 19",
    },
    {
        "name": "the corpus gives those same positions",
        "argv": ["positions", "corpus/fixtures", "cat"],
        "stdout": "doc_001 0 4 10 24\ndoc_002 15\ndoc_003 2 5 13 18 24\ndoc_004 2 6 18\ndoc_005 16\ndoc_006 18\ndoc_008 19",
    },
    {
        "name": "phrase matching still works, which needs exact positions",
        "argv": ["phrase", "build/delta-index.bin", "cat sat"],
        "stdout": "doc_001",
    },
    {
        "name": "a phrase that must not match still does not",
        "argv": ["phrase", "build/delta-index.bin", "sat cat"],
        "stdout": "",
    },
    {
        "name": "ranking is unchanged by the encoding",
        "argv": ["bm25", "build/delta-index.bin", "cat", "mat"],
        "stdout": "doc_001 7.4906\ndoc_003 2.5076\ndoc_004 2.1294\ndoc_002 1.4405\ndoc_006 1.4405\ndoc_008 1.3676\ndoc_005 1.2710",
    },
    {
        "name": "boolean evaluation is unchanged",
        "argv": ["match", "build/delta-index.bin", "cat AND NOT mat"],
        "stdout": "doc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "a single-document corpus has no gaps to encode",
        "argv": ["index-write", "--encoding", "delta", "tests/fixtures/docs", "build/delta-index.bin"],
        "stdout": "",
    },
    {
        "name": "and reads back correctly",
        "argv": ["index-terms", "build/delta-index.bin"],
        "stdout": "31 1\naspect 1\nbodi 4\ncolon 1\nfirst 1\nimmedi 1\nindex 1\nlot 1\nnoth 1\npad 1\nparagraph 1\nqueri 1\nratio 1\nsecond 1\nspace 1\nstart 1\ntalk 1\ntight 1\ntitl 1\ntwo 1",
    },
]
