#!/usr/bin/env bash
# Run the full benchmark set and print the table from BENCHMARKS.md.
#
#   ./tools/bench.sh <corpus> [query_file]
#
#   ./tools/bench.sh corpus/wikipedia/simplewiki.corpus
#   ./tools/bench.sh corpus/fixtures                     # the 30-doc fixture
#
# The corpus is indexed once to a temporary index file, and the query figures
# are measured against that index rather than against the corpus. Querying a
# corpus rebuilds the whole index first, which measures index construction and
# calls it query latency.
#
# Every figure here comes from `search bench`, `search index-size` or `time`,
# so each row can be reproduced on its own with the command shown beside it.
set -euo pipefail
cd "$(dirname "$0")/.."

CORPUS="${1:-corpus/fixtures}"
QUERIES="${2:-tests/fixtures/queries/wikipedia.txt}"
BIN=./build/search

if [ ! -x "$BIN" ]; then
  echo "build first: cmake -S . -B build && cmake --build build -j" >&2
  exit 1
fi
if [ ! -e "$CORPUS" ]; then
  echo "no such corpus: $CORPUS" >&2
  exit 1
fi
if [ ! -f "$QUERIES" ]; then
  echo "no such query file: $QUERIES" >&2
  exit 1
fi

INDEX=$(mktemp -t search-bench-XXXXXX)
trap 'rm -f "$INDEX" "$INDEX".block*' EXIT

field() { grep "^$1 " | cut -d' ' -f2-; }
cores=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)

# Peak resident memory, in bytes, for a command. GNU time reports kilobytes and
# BSD time reports bytes, so the unit has to be normalised rather than assumed.
peak_rss() {
  local out
  out=$(/usr/bin/time -l "$@" 2>&1 >/dev/null | awk '/maximum resident/ {print $1}') || true
  if [ -z "$out" ]; then
    out=$(/usr/bin/time -v "$@" 2>&1 >/dev/null \
          | awk '/Maximum resident/ {print $NF * 1024}') || true
  fi
  echo "${out:-0}"
}

echo "# Benchmark"
echo
echo "- corpus: \`$CORPUS\`"
echo "- queries: \`$QUERIES\`"
echo "- date: $(date -u '+%Y-%m-%d %H:%M UTC')"
echo "- machine: $(uname -sm)$( [ "$(uname -s)" = Darwin ] && printf ', %s' "$(sysctl -n machdep.cpu.brand_string 2>/dev/null || true)" )"
echo "- cores: $cores"
echo

echo "indexing (1 thread)..." >&2
BUILD1=$("$BIN" bench build --threads 1 "$CORPUS")
echo "indexing ($cores threads)..." >&2
BUILDN=$("$BIN" bench build --threads "$cores" "$CORPUS")

echo "writing the index..." >&2
BUILD_RSS=$(peak_rss "$BIN" index-write --threads "$cores" "$CORPUS" "$INDEX")

echo "measuring queries..." >&2
QUERY=$("$BIN" bench query --repeats 5 "$INDEX" "$QUERIES")
SNIPPET=$("$BIN" bench query --repeats 5 --snippet --corpus "$CORPUS" "$INDEX" "$QUERIES" \
          2>/dev/null || true)
SERVE_RSS=$(peak_rss "$BIN" bench query --repeats 5 "$INDEX" "$QUERIES")

echo "measuring one cold-start query..." >&2
first=$(head -n 1 "$QUERIES")
start=$(date +%s%N 2>/dev/null || echo 0)
"$BIN" query --limit 10 "$INDEX" "$first" >/dev/null
end=$(date +%s%N 2>/dev/null || echo 0)
startup_ms=$(awk -v a="$start" -v b="$end" 'BEGIN { printf "%.0f", (b - a) / 1000000 }')

echo "measuring the block build..." >&2
docs=$(echo "$BUILD1" | field documents)
block=$(( docs / 10 + 1 ))
EXT=$("$BIN" index-build --block "$block" "$CORPUS" "$INDEX.ext")
EXT_RSS=$(peak_rss "$BIN" index-build --block "$block" "$CORPUS" "$INDEX.ext")
if cmp -s "$INDEX" "$INDEX.ext"; then IDENTICAL="yes"; else IDENTICAL="NO"; fi
rm -f "$INDEX.ext" "$INDEX.ext".block*

echo "measuring compression..." >&2
SIZES=$("$BIN" index-size "$INDEX")

ms1=$(echo "$BUILD1" | field build_ms)
msN=$(echo "$BUILDN" | field build_ms)
speedup=$(awk -v a="$ms1" -v b="$msN" 'BEGIN { if (b > 0) printf "%.2f", a / b; else print "n/a" }')
gb() { awk -v b="$1" 'BEGIN { printf "%.2f", b / 1073741824 }'; }
mb() { awk -v b="$1" 'BEGIN { printf "%.0f", b / 1048576 }'; }

echo "| Measurement | Value | Reproduce with |"
echo "|---|---|---|"
echo "| Documents | $docs | \`search bench build $CORPUS\` |"
echo "| Distinct terms | $(echo "$BUILD1" | field terms) | same |"
echo "| Postings | $(echo "$BUILD1" | field postings) | same |"
echo "| Corpus bytes | $(echo "$BUILD1" | field source_bytes) | same |"
echo "| Build, 1 thread | $ms1 ms | \`search bench build --threads 1 $CORPUS\` |"
echo "| Build, $cores threads | $msN ms | \`search bench build --threads $cores $CORPUS\` |"
echo "| Parallel speedup | ${speedup}x | the two rows above |"
echo "| — corpus open | $(echo "$BUILDN" | field open_ms) ms | \`open_ms\` in the same output |"
echo "| — parallel indexing | $(echo "$BUILDN" | field index_ms) ms | \`index_ms\`, same |"
echo "| — serial merge | $(echo "$BUILDN" | field merge_ms) ms | \`merge_ms\`, same |"
echo "| Ingest rate | $(echo "$BUILDN" | field documents_per_second) docs/s, $(echo "$BUILDN" | field megabytes_per_second) MB/s | \`search bench build --threads $cores $CORPUS\` |"
echo "| Peak memory, in-memory build | $(gb "$BUILD_RSS") GB | \`/usr/bin/time -l search index-write\` |"
echo "| Peak memory, block build | $(gb "$EXT_RSS") GB | \`/usr/bin/time -l search index-build --block $block\` |"
echo "| — blocks used | $(echo "$EXT" | field blocks) | \`search index-build --block $block $CORPUS out\` |"
echo "| — byte-identical to in-memory build | $IDENTICAL | \`cmp\` of the two outputs |"
echo "| Index, uncompressed | $(echo "$SIZES" | awk '/^plain/ {print $2}') bytes | \`search index-size <index>\` |"
echo "| Index, delta + varbyte | $(echo "$SIZES" | awk '/^varbyte/ {print $2}') bytes | same |"
echo "| Compression ratio | $(echo "$SIZES" | field ratio)x | same |"
echo "| Startup, one query, whole process | ${startup_ms} ms | \`time search query <index> '$first'\` |"
echo "| Query latency p50 | $(echo "$QUERY" | field p50_ms) ms | \`search bench query --repeats 5 <index> $QUERIES\` |"
echo "| Query latency p95 | $(echo "$QUERY" | field p95_ms) ms | same |"
echo "| Query latency p99 | $(echo "$QUERY" | field p99_ms) ms | same |"
echo "| Query throughput | $(echo "$QUERY" | field queries_per_second) queries/s | same |"
if [ -n "$SNIPPET" ]; then
echo "| p50, with snippets | $(echo "$SNIPPET" | field p50_ms) ms | add \`--snippet --corpus $CORPUS\` |"
echo "| p95, with snippets | $(echo "$SNIPPET" | field p95_ms) ms | same |"
fi
echo "| Peak memory, serving | $(mb "$SERVE_RSS") MB | \`/usr/bin/time -l\` on the above |"
echo "| Queries measured | $(echo "$QUERY" | field queries) | \`search bench query\` |"
