#!/usr/bin/env python3
"""Run a query log against an index and report what came back, not just how fast.

    ./tools/analyze.py <index> [query_file] [--limit N] [--repeats N] [--scorer bm25|tfidf]

    ./tools/analyze.py corpus/wikipedia/simplewiki.idx
    ./tools/analyze.py simplewiki.idx my_queries.txt --limit 20

`search bench query` answers "how fast". This answers "how fast, and what
happened": per-query latency, how many results each query found, which queries
found nothing, and how the score distribution looks. A query that is fast
because it matched nothing is the failure this is meant to catch -- it is the
exact bug that once made this project's own benchmark report 231,481 queries a
second over an empty index.

Timings are wall clock around one subprocess per query, so they include process
startup. That is the number a person at a terminal actually waits for, and it
is deliberately not the same as the in-process figure `bench query` reports.
"""

import argparse
import pathlib
import statistics
import subprocess
import sys
import time

BINARY = pathlib.Path(__file__).resolve().parent.parent / "build" / "search"


def percentile(values, fraction):
    if not values:
        return 0.0
    ordered = sorted(values)
    at = min(int(fraction * len(ordered)), len(ordered) - 1)
    return ordered[at]


def run_query(index, query, limit, scorer):
    """One query. Returns (milliseconds, [(doc_id, score)]) or (ms, None) if rejected."""
    started = time.perf_counter()
    finished = subprocess.run(
        [str(BINARY), "query", "--tsv", "--limit", str(limit), "--scorer", scorer,
         str(index), query],
        capture_output=True, text=True,
    )
    elapsed = (time.perf_counter() - started) * 1000.0

    if finished.returncode != 0:
        return elapsed, None

    results = []
    for line in finished.stdout.splitlines():
        parts = line.split("\t")
        if len(parts) == 2:
            try:
                results.append((parts[0], float(parts[1])))
            except ValueError:
                pass
    return elapsed, results


def histogram(counts, width=40):
    """A bar per bucket, scaled to the biggest one."""
    if not counts:
        return []
    biggest = max(n for _, n in counts) or 1
    return ["  %-14s %-*s %d" % (label, width, "#" * max(1, round(n / biggest * width)) if n else "", n)
            for label, n in counts]


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument("index")
    parser.add_argument("queries", nargs="?",
                        default="tests/fixtures/queries/wikipedia.txt")
    parser.add_argument("--limit", type=int, default=10)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--scorer", choices=["bm25", "tfidf"], default="bm25")
    args = parser.parse_args()

    if not BINARY.exists():
        sys.exit("build first: cmake -S . -B build && cmake --build build -j")

    query_path = pathlib.Path(args.queries)
    if not query_path.is_file():
        sys.exit(f"no such query file: {query_path}")

    queries = [line.strip() for line in query_path.read_text().splitlines()
               if line.strip()]
    if not queries:
        sys.exit(f"no queries in {query_path}")

    print(f"index    {args.index}")
    print(f"queries  {query_path} ({len(queries)})")
    print(f"scorer   {args.scorer}, limit {args.limit}, {args.repeats} repeats")
    print()

    # One untimed pass so the page cache is warm. The first read of a large
    # index off disk is not what an interactive tool does after its first query,
    # and mixing the two into one average describes neither.
    run_query(args.index, queries[0], args.limit, args.scorer)

    rows = []
    for query in queries:
        samples = []
        results = None
        for _ in range(args.repeats):
            elapsed, got = run_query(args.index, query, args.limit, args.scorer)
            samples.append(elapsed)
            if results is None:
                results = got
        rows.append((query, statistics.median(samples), results))

    rejected = [q for q, _, r in rows if r is None]
    empty = [q for q, _, r in rows if r is not None and not r]
    answered = [(q, ms, r) for q, ms, r in rows if r]
    latencies = [ms for _, ms, _ in rows]

    print("PER QUERY")
    print("  %-34s %8s %8s %9s" % ("query", "ms", "results", "top score"))
    for query, ms, results in rows:
        if results is None:
            print("  %-34s %8.1f %8s" % (query[:34], ms, "rejected"))
        elif not results:
            print("  %-34s %8.1f %8d" % (query[:34], ms, 0))
        else:
            print("  %-34s %8.1f %8d %9.4f" % (query[:34], ms, len(results), results[0][1]))
    print()

    print("LATENCY  (wall clock per invocation, including process startup)")
    print("  p50  %8.1f ms" % percentile(latencies, 0.50))
    print("  p95  %8.1f ms" % percentile(latencies, 0.95))
    print("  p99  %8.1f ms" % percentile(latencies, 0.99))
    print("  max  %8.1f ms" % max(latencies))
    print("  mean %8.1f ms" % statistics.fmean(latencies))
    print()

    print("RESULTS")
    print("  queries answered   %d" % len(answered))
    print("  found nothing      %d%s" % (len(empty), "  <- " + ", ".join(empty[:3]) if empty else ""))
    print("  rejected by parser %d%s" % (len(rejected),
                                         "  <- " + ", ".join(rejected[:3]) if rejected else ""))
    if answered:
        filled = sum(1 for _, _, r in answered if len(r) >= args.limit)
        print("  filled the limit   %d of %d" % (filled, len(answered)))
        print("  results per query  %.1f mean, %d min, %d max" % (
            statistics.fmean(len(r) for _, _, r in answered),
            min(len(r) for _, _, r in answered),
            max(len(r) for _, _, r in answered)))
    print()

    if answered:
        tops = [r[0][1] for _, _, r in answered]
        print("SCORES  (top result of each query)")
        print("  min %.4f   median %.4f   max %.4f" % (
            min(tops), statistics.median(tops), max(tops)))

        # A top result scoring far below the rest usually means the query found
        # something technically matching and not what was asked for.
        weakest = sorted(((r[0][1], q) for q, _, r in answered))[:3]
        print("  weakest: " + ", ".join("%s (%.4f)" % (q[:24], s) for s, q in weakest))
        print()

    print("LATENCY DISTRIBUTION")
    buckets = [("< 50 ms", 0), ("50-100 ms", 0), ("100-250 ms", 0),
               ("250-500 ms", 0), ("> 500 ms", 0)]
    counts = dict(buckets)
    for ms in latencies:
        if ms < 50:
            counts["< 50 ms"] += 1
        elif ms < 100:
            counts["50-100 ms"] += 1
        elif ms < 250:
            counts["100-250 ms"] += 1
        elif ms < 500:
            counts["250-500 ms"] += 1
        else:
            counts["> 500 ms"] += 1
    for line in histogram([(label, counts[label]) for label, _ in buckets]):
        print(line)

    if empty or rejected:
        print()
        print("NOTE: %d of %d queries returned no results. A benchmark that "
              "averages these in\n      is reporting the speed of finding "
              "nothing." % (len(empty) + len(rejected), len(rows)))


if __name__ == "__main__":
    main()
