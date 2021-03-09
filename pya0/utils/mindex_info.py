import pandas as pd

MINDEX_INFO = {
    "tiny-mse": {
        "description": "Tiny toy math Q&A corpus from Math StackExchange",
        "urls": [
            "https://www.dropbox.com/s/z8hztzdksshrayy/tiny-mse.tar.gz?dl=1"
        ],
        "md5": "cb54659c7059dff42935eb67fb953dc1",
        "size compressed (bytes)": 703411200,
        "documents": 68,
        "corpus_url": "https://github.com/approach0/search-engine/tree/ecir2020/indexer/test-corpus/mse-small",
    },
    "mse-ecir2020": {
        "description": "Math Q&A corpus used to evaluate efficiency in ECIR 2020",
        "urls": [
			"https://www.dropbox.com/s/w1qco7s49oren9y/mse-ecir2020.tar.gz?dl=1"
        ],
        "md5": "35bcbc0f144ff4dd8baf35d439a6cc28",
        "size compressed (bytes)": 5654523904,
        "documents": 1057449,
        "corpus_url": "https://www.cs.rit.edu/~dprl/data/mse-corpus.tar.gz",
        "image_content_size": "8.2G",
        "image_filesystem": "reiserfs",
    },
    "ntcir12-math-browsing": {
        "description": "NTCIR-12 Wikipedia dataset",
        "urls": [
            "https://www.dropbox.com/s/s8xr0fi3lmvtxn4/ntcir12-math-browsing.tar.gz?dl=1",
        ],
        "md5": "5312c919ba92cd3fbb4b6de6e43551f3",
        "size compressed (bytes)": 163893248,
        "documents": 591468,
        "corpus_url": "https://drive.google.com/file/d/1kaNjozKJhDUnCevw4fsrmYdsw8Wixkn2/view?usp=sharing",
        "image_content_size": "609M",
        "image_filesystem": "reiserfs",
    },
    "arqmath-2020-task1": {
        "description": "CLEF ARQMath 2020 task2 (posts from MSE in the years 2010 to 2018)",
        "urls": [
            "https://www.dropbox.com/s/ofydiw10yf2ursa/arqmath-2020-task1.tar.gz?dl=1",
        ],
        "md5": "6d0894f01bb40c2b06c5300f48d6fc0b",
        "size compressed (bytes)": 10294255616,
        "documents":  1445495, # Number of posts here, there are 1 million documents though.
        "corpus_url": "https://drive.google.com/file/d/1q595qXOdi1eHbC5RxMULMnbtYG4ukK3C/view?usp=sharing",
        "corpus_image_minimal_size": "3G",
        "image_content_size": "5.6G",
        "image_filesystem": "reiserfs",
    },
    "arqmath-2020-task2-part1": {
        "description": "CLEF ARQMath 2020 task2 (math formulas from MSE in the years 2010 to 2018) [part1]",
        "urls": [
            "https://www.dropbox.com/s/79amy930x5cdv5p/arqmath-2020-task2-part1.tar.gz?dl=1",
        ],
        "md5": "f140d94178dad084955ebd646a831a7e",
        "size compressed (bytes)": 2071553043,
        "documents": 28320920, # 22980699 multi-token formulas
        "corpus_url": "https://drive.google.com/file/d/1rtINvwODOK-YH79OleoVYH90X9W3m1Q_/view?usp=sharing",
        "corpus_image_minimal_size": "4.9G",
        "image_content_size": "3.0G",
        "image_filesystem": "reiserfs",
    },
    "arqmath-2020-task2-part2": {
        "description": "CLEF ARQMath 2020 task2 (math formulas from MSE in the years 2010 to 2018) [part2]",
        "urls": [
            "https://www.dropbox.com/s/8loo9cwmjrua94k/arqmath-2020-task2-part2.tar.gz?dl=1",
        ],
        "md5": "05618cddb0131716ee94c26f8419e534",
        "size compressed (bytes)": 2069404794,
        "documents": 28320920, # 22980699 multi-token formulas
        "corpus_url": "https://drive.google.com/file/d/1rtINvwODOK-YH79OleoVYH90X9W3m1Q_/view?usp=sharing",
        "corpus_image_minimal_size": "4.9G",
        "image_content_size": "3.0G",
        "image_filesystem": "reiserfs",
    },
    "arqmath-2020-task2-part3": {
        "description": "CLEF ARQMath 2020 task2 (math formulas from MSE in the years 2010 to 2018) [part3]",
        "urls": [
            "https://www.dropbox.com/s/ky5j2v75melz0ks/arqmath-2020-task2-part3.tar.gz?dl=1",
        ],
        "md5": "e155d89feeca5bf7b4b90d1d5844ec26",
        "size compressed (bytes)": 2050106292,
        "documents": 28320920, # 22980699 multi-token formulas
        "corpus_url": "https://drive.google.com/file/d/1rtINvwODOK-YH79OleoVYH90X9W3m1Q_/view?usp=sharing",
        "corpus_image_minimal_size": "4.9G",
        "image_content_size": "3.0G",
        "image_filesystem": "reiserfs",
    },
    "arqmath-2020-task2-part4": {
        "description": "CLEF ARQMath 2020 task2 (math formulas from MSE in the years 2010 to 2018) [part4]",
        "urls": [
            "https://www.dropbox.com/s/d1pxlabjdbojdp0/arqmath-2020-task2-part4.tar.gz?dl=1",
        ],
        "md5": "c7d84f05342097b4db28149af4cd1f76",
        "size compressed (bytes)": 2075163474,
        "documents": 28320920, # 22980699 multi-token formulas
        "corpus_url": "https://drive.google.com/file/d/1rtINvwODOK-YH79OleoVYH90X9W3m1Q_/view?usp=sharing",
        "corpus_image_minimal_size": "4.9G",
        "image_content_size": "3.0G",
        "image_filesystem": "reiserfs",
    },
    "ecir2020-a0-arqmath2020-task1": {
        "description": "To reproduce our submission at ARQMath 2020 for task 1 (with single-token math treated as text)",
        "urls": [
            "",
        ],
        "md5": "980337363e92bd02eacdb96db1869a27",
        "size compressed (bytes)": 11193804419,
        "image_content_size": "13G",
        "image_filesystem": "reiserfs",
	},
    "ecir2020-a0-mse": {
        "description": "To reproduce our ECIR2020 efficiency results on ad-hoc MSE corpus",
        "urls": [
            "",
        ],
        "md5": "f4193a4b35bb302f2a2382bbf18450d0",
        "size compressed (bytes)": 9183807154,
        "image_content_size": "16G",
        "image_filesystem": "reiserfs",
	},
}

def list_indexes():
    df = pd.DataFrame.from_dict(MINDEX_INFO)
    with pd.option_context('display.max_rows', None, 'display.max_columns',
                           None, 'display.max_colwidth', -1, 'display.colheader_justify', 'left'):
        print(df)
