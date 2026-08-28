"""`search corpus-write` and reading a .corpus anywhere a directory works."""

NAME = "Packed corpus files"
ORDER = 470

CASES = [
    {
        "name": "packing a corpus produces no output",
        "argv": ["corpus-write", "corpus/fixtures", "build/test-fixtures.corpus"],
        "stdout": "",
    },
    {
        "name": "the packed corpus lists the same documents",
        "argv": ["docs", "build/test-fixtures.corpus"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_007\ndoc_008\ndoc_009\ndoc_010\ndoc_011\ndoc_012\ndoc_013\ndoc_014\ndoc_015\ndoc_016\ndoc_017\ndoc_018\ndoc_019\ndoc_020\ndoc_021\ndoc_022\ndoc_023\ndoc_024\ndoc_025\ndoc_026\ndoc_027\ndoc_028\ndoc_029\ndoc_030",
    },
    {
        "name": "as does the directory",
        "argv": ["docs", "corpus/fixtures"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_007\ndoc_008\ndoc_009\ndoc_010\ndoc_011\ndoc_012\ndoc_013\ndoc_014\ndoc_015\ndoc_016\ndoc_017\ndoc_018\ndoc_019\ndoc_020\ndoc_021\ndoc_022\ndoc_023\ndoc_024\ndoc_025\ndoc_026\ndoc_027\ndoc_028\ndoc_029\ndoc_030",
    },
    {
        "name": "a document reads back identically",
        "argv": ["show", "build/test-fixtures.corpus", "doc_001"],
        "stdout": "id: doc_001\ntitle: Cats and Mats\n\nThe cat sat on the mat. The cat was happy on that mat. A mat is a flat rug and the cat likes to sit on it every afternoon.",
    },
    {
        "name": "and matches the directory",
        "argv": ["show", "corpus/fixtures", "doc_001"],
        "stdout": "id: doc_001\ntitle: Cats and Mats\n\nThe cat sat on the mat. The cat was happy on that mat. A mat is a flat rug and the cat likes to sit on it every afternoon.",
    },
    {
        "name": "indexing a packed corpus gives the same counts",
        "argv": ["index-stats", "build/test-fixtures.corpus"],
        "stdout": "documents: 30\nterms: 235\npostings: 343",
    },
    {
        "name": "as indexing the directory",
        "argv": ["index-stats", "corpus/fixtures"],
        "stdout": "documents: 30\nterms: 235\npostings: 343",
    },
    {
        "name": "the dictionary is the same",
        "argv": ["index-terms", "build/test-fixtures.corpus"],
        "stdout": "across 3\nadd 2\nafternoon 1\nair 1\nalgorithm 1\nanim 3\naround 1\narrai 1\nask 1\nastronom 1\nawai 1\nback 1\nbake 1\nbaker 1\nbark 1\nbean 1\nbillion 1\nbinari 1\nbird 1\nblack 1\nboil 1\nbread 1\nbring 1\nbrush 1\nburn 3\nbutter 1\ncare 1\ncarrot 1\ncat 7\ncentr 1\ncheap 1\ncode 3\ncold 1\ncollect 1\ncommon 1\ncompil 1\ncomput 3\ncontain 1\ncook 5\ncover 1\ncrack 1\ncrater 1\ncross 1\ncut 2\ndai 4\ndaili 1\ndata 3\ndatabas 1\ndefin 1\ndi 1\ndistant 1\ndocument 1\ndog 3\ndough 1\ndrain 1\ndrive 1\ndull 1\ndust 1\nearth 4\neat 1\negg 1\nenjoi 1\nescap 1\nevenli 1\neveri 7\nexercis 1\nfainter 1\nfarm 1\nfast 1\nfaster 2\nfeed 1\nfield 1\nfill 1\nfire 1\nfirm 1\nflat 1\nflavour 1\nflour 1\nfly 2\nfood 1\nforti 1\nfresh 1\nfry 1\nfuel 1\nfunction 1\ngalaxi 1\ngarden 1\ngive 1\ngood 4\ngraviti 1\ngrei 1\ngrill 1\nhalf 1\nhalv 1\nhappi 1\nhard 1\nhash 1\nheat 2\nhold 2\nhors 1\nhot 2\nhour 1\nhous 3\nhunt 2\nhunter 1\nhydrogen 1\nimag 1\nimport 1\nindex 1\nkeep 1\nkei 2\nknife 1\nknive 1\nlarg 5\nlarger 1\nlaunch 1\nlearn 1\nlet 1\nlight 2\nlike 1\nlive 2\nmake 3\nmap 2\nmar 1\nmat 1\nmeal 2\nmemori 2\nmerg 1\nmice 1\nminut 3\nmix 1\nmonth 1\nmoon 1\nmorn 1\nmous 2\nmove 1\nnear 2\nneed 3\nnight 2\nobject 1\noil 1\nold 4\non 1\nonion 2\norbit 2\noven 1\npan 1\nparallel 1\npark 1\npasta 1\npet 1\nplan 1\nplanet 3\nponi 1\npot 1\npractic 1\nprocessor 1\nproduc 1\nprogram 4\nprogramm 1\npurr 1\nqueri 1\nquick 1\nquickli 1\nquiet 2\nread 3\nred 1\nrelat 1\nrice 1\nroad 1\nrocket 1\nrover 1\nrug 1\nrun 4\nrunner 1\nsafer 1\nsalt 1\nsat 1\nsatellit 1\nsauc 1\nsearch 1\nsee 1\nsend 1\nshare 1\nsharp 1\nsharpen 1\nsimmer 2\nsimpl 1\nsing 1\nsit 1\nslip 1\nslow 2\nslowli 1\nsmall 4\nsolar 1\nsomewher 1\nsort 1\nsoup 1\nsourc 1\nspice 1\nsplit 1\nspread 1\nstar 4\nstart 2\nstore 2\nstudi 1\nsun 1\nsurfac 2\nsystem 1\ntabl 2\ntake 2\ntask 2\ntelescop 1\nten 1\ntext 1\nthread 1\ntogeth 2\ntrap 1\ntwice 1\ntwo 2\nvalu 1\nveget 2\nviolent 1\nwai 1\nwall 1\nwatch 1\nwater 6\nweather 1\nwhite 1\nwindow 1\nwithout 2\nword 1\nwrite 1\nyear 2\nyeast 1",
    },
    {
        "name": "document lengths are the same, so ordinals match",
        "argv": ["lengths", "build/test-fixtures.corpus"],
        "stdout": "doc_001 16\ndoc_002 15\ndoc_003 16\ndoc_004 19\ndoc_005 20\ndoc_006 15\ndoc_007 15\ndoc_008 17\ndoc_009 15\ndoc_010 14\ndoc_011 16\ndoc_012 17\ndoc_013 14\ndoc_014 15\ndoc_015 15\ndoc_016 14\ndoc_017 18\ndoc_018 17\ndoc_019 16\ndoc_020 16\ndoc_021 13\ndoc_022 15\ndoc_023 16\ndoc_024 14\ndoc_025 17\ndoc_026 13\ndoc_027 15\ndoc_028 14\ndoc_029 15\ndoc_030 15\naverage 15.57",
    },
    {
        "name": "positions survive packing",
        "argv": ["positions", "build/test-fixtures.corpus", "cat"],
        "stdout": "doc_001 0 4 10 24\ndoc_002 15\ndoc_003 2 5 13 18 24\ndoc_004 2 6 18\ndoc_005 16\ndoc_006 18\ndoc_008 19",
    },
    {
        "name": "boolean queries work against a packed corpus",
        "argv": ["match", "build/test-fixtures.corpus", "cat AND NOT mat"],
        "stdout": "doc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "phrase queries work too",
        "argv": ["phrase", "build/test-fixtures.corpus", "cat sat"],
        "stdout": "doc_001",
    },
    {
        "name": "ranking is unchanged",
        "argv": ["bm25", "build/test-fixtures.corpus", "cat", "mat"],
        "stdout": "doc_001 7.4906\ndoc_003 2.5076\ndoc_004 2.1294\ndoc_002 1.4405\ndoc_006 1.4405\ndoc_008 1.3676\ndoc_005 1.2710",
    },
    {
        "name": "snippets work, since the container keeps the text",
        "argv": ["snippet", "build/test-fixtures.corpus", "doc_001", "cats", "mat"],
        "stdout": "[Cats] and [Mats]\n\nThe [cat] sat on the [mat]. The [cat] was happy on that [mat]. A [mat] is a flat rug and the [cat] likes to sit on it every afternoon.",
    },
    {
        "name": "analyze-doc works",
        "argv": ["analyze-doc", "build/test-fixtures.corpus", "doc_001"],
        "stdout": "0 0 cat\n2 9 mat\n4 19 cat\n5 23 sat\n8 34 mat\n10 43 cat\n12 51 happi\n15 65 mat\n17 72 mat\n20 81 flat\n21 86 rug\n24 98 cat\n25 102 like\n27 111 sit\n30 121 everi\n31 127 afternoon",
    },
    {
        "name": "the tiny fixture packed at build time",
        "argv": ["docs", "tests/fixtures/corpusfile/tiny.corpus"],
        "stdout": "alpha\nbeta\ngamma",
    },
    {
        "name": "and indexed",
        "argv": ["index-stats", "tests/fixtures/corpusfile/tiny.corpus"],
        "stdout": "documents: 3\nterms: 11\npostings: 19",
    },
    {
        "name": "packing an empty corpus is legal",
        "argv": ["corpus-write", "tests/fixtures/empty", "build/test-empty.corpus"],
        "stdout": "",
    },
    {
        "name": "and yields nothing",
        "argv": ["docs", "build/test-empty.corpus"],
        "stdout": "",
    },
    {
        "name": "malformed documents are packed but skipped at index time",
        "argv": ["corpus-write", "tests/fixtures/docs", "build/test-docs.corpus"],
        "stdout": "",
    },
    {
        "name": "just as they are from the directory",
        "argv": ["index-stats", "build/test-docs.corpus"],
        "stdout": "documents: 7\nterms: 20\npostings: 23",
    },
    {
        "name": "a file that is not a corpus is rejected",
        "argv": ["docs", "tests/fixtures/corpusfile/bad_magic.corpus"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unknown version is rejected",
        "argv": ["docs", "tests/fixtures/corpusfile/bad_version.corpus"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a truncated file is rejected",
        "argv": ["docs", "tests/fixtures/corpusfile/truncated.corpus"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "trailing bytes are rejected",
        "argv": ["docs", "tests/fixtures/corpusfile/trailing.corpus"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an empty file is rejected",
        "argv": ["docs", "tests/fixtures/corpusfile/empty.corpus"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a table offset past the end is rejected",
        "argv": ["docs", "tests/fixtures/corpusfile/bad_offset.corpus"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "packing a missing directory is reported",
        "argv": ["corpus-write", "tests/fixtures/nope", "build/test-fixtures.corpus"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "packing to an unwritable path is reported",
        "argv": ["corpus-write", "corpus/fixtures", "/nope/x.corpus"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "corpus-write with no destination is rejected",
        "argv": ["corpus-write", "corpus/fixtures"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
