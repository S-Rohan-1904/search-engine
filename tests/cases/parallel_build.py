"""`search index-write --threads <n>` -- building on several threads must match building on one."""

NAME = "Parallel index construction"
ORDER = 320

CASES = [
    {
        "name": "a single-threaded build",
        "argv": ["index-write", "--threads", "1", "corpus/fixtures", "build/parallel-index.bin"],
        "stdout": "",
    },
    {
        "name": "its dictionary",
        "argv": ["index-terms", "build/parallel-index.bin"],
        "stdout": "across 3\nadd 2\nafternoon 1\nair 1\nalgorithm 1\nanim 3\naround 1\narrai 1\nask 1\nastronom 1\nawai 1\nback 1\nbake 1\nbaker 1\nbark 1\nbean 1\nbillion 1\nbinari 1\nbird 1\nblack 1\nboil 1\nbread 1\nbring 1\nbrush 1\nburn 3\nbutter 1\ncare 1\ncarrot 1\ncat 7\ncentr 1\ncheap 1\ncode 3\ncold 1\ncollect 1\ncommon 1\ncompil 1\ncomput 3\ncontain 1\ncook 5\ncover 1\ncrack 1\ncrater 1\ncross 1\ncut 2\ndai 4\ndaili 1\ndata 3\ndatabas 1\ndefin 1\ndi 1\ndistant 1\ndocument 1\ndog 3\ndough 1\ndrain 1\ndrive 1\ndull 1\ndust 1\nearth 4\neat 1\negg 1\nenjoi 1\nescap 1\nevenli 1\neveri 7\nexercis 1\nfainter 1\nfarm 1\nfast 1\nfaster 2\nfeed 1\nfield 1\nfill 1\nfire 1\nfirm 1\nflat 1\nflavour 1\nflour 1\nfly 2\nfood 1\nforti 1\nfresh 1\nfry 1\nfuel 1\nfunction 1\ngalaxi 1\ngarden 1\ngive 1\ngood 4\ngraviti 1\ngrei 1\ngrill 1\nhalf 1\nhalv 1\nhappi 1\nhard 1\nhash 1\nheat 2\nhold 2\nhors 1\nhot 2\nhour 1\nhous 3\nhunt 2\nhunter 1\nhydrogen 1\nimag 1\nimport 1\nindex 1\nkeep 1\nkei 2\nknife 1\nknive 1\nlarg 5\nlarger 1\nlaunch 1\nlearn 1\nlet 1\nlight 2\nlike 1\nlive 2\nmake 3\nmap 2\nmar 1\nmat 1\nmeal 2\nmemori 2\nmerg 1\nmice 1\nminut 3\nmix 1\nmonth 1\nmoon 1\nmorn 1\nmous 2\nmove 1\nnear 2\nneed 3\nnight 2\nobject 1\noil 1\nold 4\non 1\nonion 2\norbit 2\noven 1\npan 1\nparallel 1\npark 1\npasta 1\npet 1\nplan 1\nplanet 3\nponi 1\npot 1\npractic 1\nprocessor 1\nproduc 1\nprogram 4\nprogramm 1\npurr 1\nqueri 1\nquick 1\nquickli 1\nquiet 2\nread 3\nred 1\nrelat 1\nrice 1\nroad 1\nrocket 1\nrover 1\nrug 1\nrun 4\nrunner 1\nsafer 1\nsalt 1\nsat 1\nsatellit 1\nsauc 1\nsearch 1\nsee 1\nsend 1\nshare 1\nsharp 1\nsharpen 1\nsimmer 2\nsimpl 1\nsing 1\nsit 1\nslip 1\nslow 2\nslowli 1\nsmall 4\nsolar 1\nsomewher 1\nsort 1\nsoup 1\nsourc 1\nspice 1\nsplit 1\nspread 1\nstar 4\nstart 2\nstore 2\nstudi 1\nsun 1\nsurfac 2\nsystem 1\ntabl 2\ntake 2\ntask 2\ntelescop 1\nten 1\ntext 1\nthread 1\ntogeth 2\ntrap 1\ntwice 1\ntwo 2\nvalu 1\nveget 2\nviolent 1\nwai 1\nwall 1\nwatch 1\nwater 6\nweather 1\nwhite 1\nwindow 1\nwithout 2\nword 1\nwrite 1\nyear 2\nyeast 1",
    },
    {
        "name": "a four-threaded build of the same corpus",
        "argv": ["index-write", "--threads", "4", "corpus/fixtures", "build/parallel-index.bin"],
        "stdout": "",
    },
    {
        "name": "gives the same dictionary",
        "argv": ["index-terms", "build/parallel-index.bin"],
        "stdout": "across 3\nadd 2\nafternoon 1\nair 1\nalgorithm 1\nanim 3\naround 1\narrai 1\nask 1\nastronom 1\nawai 1\nback 1\nbake 1\nbaker 1\nbark 1\nbean 1\nbillion 1\nbinari 1\nbird 1\nblack 1\nboil 1\nbread 1\nbring 1\nbrush 1\nburn 3\nbutter 1\ncare 1\ncarrot 1\ncat 7\ncentr 1\ncheap 1\ncode 3\ncold 1\ncollect 1\ncommon 1\ncompil 1\ncomput 3\ncontain 1\ncook 5\ncover 1\ncrack 1\ncrater 1\ncross 1\ncut 2\ndai 4\ndaili 1\ndata 3\ndatabas 1\ndefin 1\ndi 1\ndistant 1\ndocument 1\ndog 3\ndough 1\ndrain 1\ndrive 1\ndull 1\ndust 1\nearth 4\neat 1\negg 1\nenjoi 1\nescap 1\nevenli 1\neveri 7\nexercis 1\nfainter 1\nfarm 1\nfast 1\nfaster 2\nfeed 1\nfield 1\nfill 1\nfire 1\nfirm 1\nflat 1\nflavour 1\nflour 1\nfly 2\nfood 1\nforti 1\nfresh 1\nfry 1\nfuel 1\nfunction 1\ngalaxi 1\ngarden 1\ngive 1\ngood 4\ngraviti 1\ngrei 1\ngrill 1\nhalf 1\nhalv 1\nhappi 1\nhard 1\nhash 1\nheat 2\nhold 2\nhors 1\nhot 2\nhour 1\nhous 3\nhunt 2\nhunter 1\nhydrogen 1\nimag 1\nimport 1\nindex 1\nkeep 1\nkei 2\nknife 1\nknive 1\nlarg 5\nlarger 1\nlaunch 1\nlearn 1\nlet 1\nlight 2\nlike 1\nlive 2\nmake 3\nmap 2\nmar 1\nmat 1\nmeal 2\nmemori 2\nmerg 1\nmice 1\nminut 3\nmix 1\nmonth 1\nmoon 1\nmorn 1\nmous 2\nmove 1\nnear 2\nneed 3\nnight 2\nobject 1\noil 1\nold 4\non 1\nonion 2\norbit 2\noven 1\npan 1\nparallel 1\npark 1\npasta 1\npet 1\nplan 1\nplanet 3\nponi 1\npot 1\npractic 1\nprocessor 1\nproduc 1\nprogram 4\nprogramm 1\npurr 1\nqueri 1\nquick 1\nquickli 1\nquiet 2\nread 3\nred 1\nrelat 1\nrice 1\nroad 1\nrocket 1\nrover 1\nrug 1\nrun 4\nrunner 1\nsafer 1\nsalt 1\nsat 1\nsatellit 1\nsauc 1\nsearch 1\nsee 1\nsend 1\nshare 1\nsharp 1\nsharpen 1\nsimmer 2\nsimpl 1\nsing 1\nsit 1\nslip 1\nslow 2\nslowli 1\nsmall 4\nsolar 1\nsomewher 1\nsort 1\nsoup 1\nsourc 1\nspice 1\nsplit 1\nspread 1\nstar 4\nstart 2\nstore 2\nstudi 1\nsun 1\nsurfac 2\nsystem 1\ntabl 2\ntake 2\ntask 2\ntelescop 1\nten 1\ntext 1\nthread 1\ntogeth 2\ntrap 1\ntwice 1\ntwo 2\nvalu 1\nveget 2\nviolent 1\nwai 1\nwall 1\nwatch 1\nwater 6\nweather 1\nwhite 1\nwindow 1\nwithout 2\nword 1\nwrite 1\nyear 2\nyeast 1",
    },
    {
        "name": "and the same counts",
        "argv": ["index-stats", "build/parallel-index.bin"],
        "stdout": "documents: 30\nterms: 235\npostings: 343",
    },
    {
        "name": "and the same document ordinals, which lengths expose",
        "argv": ["lengths", "build/parallel-index.bin"],
        "stdout": "doc_001 16\ndoc_002 15\ndoc_003 16\ndoc_004 19\ndoc_005 20\ndoc_006 15\ndoc_007 15\ndoc_008 17\ndoc_009 15\ndoc_010 14\ndoc_011 16\ndoc_012 17\ndoc_013 14\ndoc_014 15\ndoc_015 15\ndoc_016 14\ndoc_017 18\ndoc_018 17\ndoc_019 16\ndoc_020 16\ndoc_021 13\ndoc_022 15\ndoc_023 16\ndoc_024 14\ndoc_025 17\ndoc_026 13\ndoc_027 15\ndoc_028 14\ndoc_029 15\ndoc_030 15\naverage 15.57",
    },
    {
        "name": "and the same postings",
        "argv": ["positions", "build/parallel-index.bin", "cat"],
        "stdout": "doc_001 0 4 10 24\ndoc_002 15\ndoc_003 2 5 13 18 24\ndoc_004 2 6 18\ndoc_005 16\ndoc_006 18\ndoc_008 19",
    },
    {
        "name": "sixteen threads on thirty documents",
        "argv": ["index-write", "--threads", "16", "corpus/fixtures", "build/parallel-index.bin"],
        "stdout": "",
    },
    {
        "name": "still gives the same postings",
        "argv": ["positions", "build/parallel-index.bin", "cat"],
        "stdout": "doc_001 0 4 10 24\ndoc_002 15\ndoc_003 2 5 13 18 24\ndoc_004 2 6 18\ndoc_005 16\ndoc_006 18\ndoc_008 19",
    },
    {
        "name": "more threads than documents",
        "argv": ["index-write", "--threads", "16", "tests/fixtures/tiny", "build/parallel-index.bin"],
        "stdout": "",
    },
    {
        "name": "still indexes all three",
        "argv": ["index-stats", "build/parallel-index.bin"],
        "stdout": "documents: 3\nterms: 11\npostings: 19",
    },
    {
        "name": "a corpus with malformed documents keeps its ordinals",
        "argv": ["index-write", "--threads", "4", "tests/fixtures/docs", "build/parallel-index.bin"],
        "stdout": "",
    },
    {
        "name": "which lengths confirm",
        "argv": ["lengths", "build/parallel-index.bin"],
        "stdout": "colon_title 5\nempty_body 1\nempty_title 2\nextra_blanks 3\nmulti_para 10\nno_blank_line 3\nspaced_title 3\naverage 3.86",
    },
    {
        "name": "an empty corpus builds on many threads",
        "argv": ["index-write", "--threads", "8", "tests/fixtures/empty", "build/parallel-index.bin"],
        "stdout": "",
    },
    {
        "name": "and is still empty",
        "argv": ["index-stats", "build/parallel-index.bin"],
        "stdout": "documents: 0\nterms: 0\npostings: 0",
    },
    {
        "name": "zero threads is rejected",
        "argv": ["index-write", "--threads", "0", "corpus/fixtures", "build/parallel-index.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a non-numeric thread count is rejected",
        "argv": ["index-write", "--threads", "x", "corpus/fixtures", "build/parallel-index.bin"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
]
