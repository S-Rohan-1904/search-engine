"""Variable-byte encoding stores small numbers in fewer bytes, and must read back identically."""

NAME = "Variable-byte encoding"
ORDER = 290

CASES = [
    {
        "name": "a varbyte-encoded index of the working corpus is written",
        "argv": ["index-write", "--encoding", "varbyte", "corpus/fixtures", "build/varbyte-index.bin"],
        "stdout": "",
    },
    {
        "name": "its counts match the corpus",
        "argv": ["index-stats", "build/varbyte-index.bin"],
        "stdout": "documents: 30\nterms: 235\npostings: 343",
    },
    {
        "name": "the dictionary survives byte-level packing",
        "argv": ["index-terms", "build/varbyte-index.bin"],
        "stdout": "across 3\nadd 2\nafternoon 1\nair 1\nalgorithm 1\nanim 3\naround 1\narrai 1\nask 1\nastronom 1\nawai 1\nback 1\nbake 1\nbaker 1\nbark 1\nbean 1\nbillion 1\nbinari 1\nbird 1\nblack 1\nboil 1\nbread 1\nbring 1\nbrush 1\nburn 3\nbutter 1\ncare 1\ncarrot 1\ncat 7\ncentr 1\ncheap 1\ncode 3\ncold 1\ncollect 1\ncommon 1\ncompil 1\ncomput 3\ncontain 1\ncook 5\ncover 1\ncrack 1\ncrater 1\ncross 1\ncut 2\ndai 4\ndaili 1\ndata 3\ndatabas 1\ndefin 1\ndi 1\ndistant 1\ndocument 1\ndog 3\ndough 1\ndrain 1\ndrive 1\ndull 1\ndust 1\nearth 4\neat 1\negg 1\nenjoi 1\nescap 1\nevenli 1\neveri 7\nexercis 1\nfainter 1\nfarm 1\nfast 1\nfaster 2\nfeed 1\nfield 1\nfill 1\nfire 1\nfirm 1\nflat 1\nflavour 1\nflour 1\nfly 2\nfood 1\nforti 1\nfresh 1\nfry 1\nfuel 1\nfunction 1\ngalaxi 1\ngarden 1\ngive 1\ngood 4\ngraviti 1\ngrei 1\ngrill 1\nhalf 1\nhalv 1\nhappi 1\nhard 1\nhash 1\nheat 2\nhold 2\nhors 1\nhot 2\nhour 1\nhous 3\nhunt 2\nhunter 1\nhydrogen 1\nimag 1\nimport 1\nindex 1\nkeep 1\nkei 2\nknife 1\nknive 1\nlarg 5\nlarger 1\nlaunch 1\nlearn 1\nlet 1\nlight 2\nlike 1\nlive 2\nmake 3\nmap 2\nmar 1\nmat 1\nmeal 2\nmemori 2\nmerg 1\nmice 1\nminut 3\nmix 1\nmonth 1\nmoon 1\nmorn 1\nmous 2\nmove 1\nnear 2\nneed 3\nnight 2\nobject 1\noil 1\nold 4\non 1\nonion 2\norbit 2\noven 1\npan 1\nparallel 1\npark 1\npasta 1\npet 1\nplan 1\nplanet 3\nponi 1\npot 1\npractic 1\nprocessor 1\nproduc 1\nprogram 4\nprogramm 1\npurr 1\nqueri 1\nquick 1\nquickli 1\nquiet 2\nread 3\nred 1\nrelat 1\nrice 1\nroad 1\nrocket 1\nrover 1\nrug 1\nrun 4\nrunner 1\nsafer 1\nsalt 1\nsat 1\nsatellit 1\nsauc 1\nsearch 1\nsee 1\nsend 1\nshare 1\nsharp 1\nsharpen 1\nsimmer 2\nsimpl 1\nsing 1\nsit 1\nslip 1\nslow 2\nslowli 1\nsmall 4\nsolar 1\nsomewher 1\nsort 1\nsoup 1\nsourc 1\nspice 1\nsplit 1\nspread 1\nstar 4\nstart 2\nstore 2\nstudi 1\nsun 1\nsurfac 2\nsystem 1\ntabl 2\ntake 2\ntask 2\ntelescop 1\nten 1\ntext 1\nthread 1\ntogeth 2\ntrap 1\ntwice 1\ntwo 2\nvalu 1\nveget 2\nviolent 1\nwai 1\nwall 1\nwatch 1\nwater 6\nweather 1\nwhite 1\nwindow 1\nwithout 2\nword 1\nwrite 1\nyear 2\nyeast 1",
    },
    {
        "name": "postings decode correctly",
        "argv": ["postings", "build/varbyte-index.bin", "cat"],
        "stdout": "doc_001\ndoc_002\ndoc_003\ndoc_004\ndoc_005\ndoc_006\ndoc_008",
    },
    {
        "name": "frequencies decode correctly",
        "argv": ["tf", "build/varbyte-index.bin", "cat"],
        "stdout": "doc_001 4\ndoc_002 1\ndoc_003 5\ndoc_004 3\ndoc_005 1\ndoc_006 1\ndoc_008 1",
    },
    {
        "name": "positions decode correctly",
        "argv": ["positions", "build/varbyte-index.bin", "cat"],
        "stdout": "doc_001 0 4 10 24\ndoc_002 15\ndoc_003 2 5 13 18 24\ndoc_004 2 6 18\ndoc_005 16\ndoc_006 18\ndoc_008 19",
    },
    {
        "name": "document lengths decode correctly",
        "argv": ["lengths", "build/varbyte-index.bin"],
        "stdout": "doc_001 16\ndoc_002 15\ndoc_003 16\ndoc_004 19\ndoc_005 20\ndoc_006 15\ndoc_007 15\ndoc_008 17\ndoc_009 15\ndoc_010 14\ndoc_011 16\ndoc_012 17\ndoc_013 14\ndoc_014 15\ndoc_015 15\ndoc_016 14\ndoc_017 18\ndoc_018 17\ndoc_019 16\ndoc_020 16\ndoc_021 13\ndoc_022 15\ndoc_023 16\ndoc_024 14\ndoc_025 17\ndoc_026 13\ndoc_027 15\ndoc_028 14\ndoc_029 15\ndoc_030 15\naverage 15.57",
    },
    {
        "name": "phrase matching still works",
        "argv": ["phrase", "build/varbyte-index.bin", "sun is a star"],
        "stdout": "doc_026",
    },
    {
        "name": "a near-miss phrase still misses",
        "argv": ["phrase", "build/varbyte-index.bin", "sun star"],
        "stdout": "",
    },
    {
        "name": "ranking is unchanged",
        "argv": ["bm25", "build/varbyte-index.bin", "cat", "mat"],
        "stdout": "doc_001 7.4906\ndoc_003 2.5076\ndoc_004 2.1294\ndoc_002 1.4405\ndoc_006 1.4405\ndoc_008 1.3676\ndoc_005 1.2710",
    },
    {
        "name": "top-k is unchanged",
        "argv": ["top", "--bm25", "build/varbyte-index.bin", "3", "cat"],
        "stdout": "doc_003 2.5076\ndoc_001 2.3900\ndoc_004 2.1294",
    },
    {
        "name": "an empty corpus encodes and decodes",
        "argv": ["index-write", "--encoding", "varbyte", "tests/fixtures/empty", "build/varbyte-index.bin"],
        "stdout": "",
    },
    {
        "name": "and reports nothing",
        "argv": ["index-stats", "build/varbyte-index.bin"],
        "stdout": "documents: 0\nterms: 0\npostings: 0",
    },
]
