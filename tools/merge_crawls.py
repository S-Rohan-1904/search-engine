#!/usr/bin/env python3
import shutil
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) < 3:
        print("usage: merge_crawls.py <out_dir> <crawl_dir>...", file=sys.stderr)
        return 2

    out = Path(sys.argv[1])
    sources = [Path(p) for p in sys.argv[2:]]
    if out.exists():
        print(f"refusing to overwrite {out}", file=sys.stderr)
        return 1
    out.mkdir(parents=True)

    links, urls, documents = [], [], 0
    for source in sources:
        prefix = source.name
        for page in sorted(source.glob("doc_*.txt")):
            shutil.copyfile(page, out / f"{prefix}_{page.name}")
            documents += 1
        for line in (source / "links.tsv").read_text().splitlines():
            a, b = line.split("\t")
            links.append(f"{prefix}_{a}\t{prefix}_{b}\n")
        for line in (source / "urls.tsv").read_text().splitlines():
            doc, url = line.split("\t", 1)
            urls.append(f"{prefix}_{doc}\t{url}\n")

    (out / "links.tsv").write_text("".join(links))
    (out / "urls.tsv").write_text("".join(urls))
    print(f"documents {documents}\nlinks {len(links)}\nsites {len(sources)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
