#!/usr/bin/env bash
# Run the standard benchmark set and print a table.
#
#   ./tools/bench.sh <source> [query_file]
#
# Every figure here comes from `search bench`, `search index-size` or `time`, so
# each row can be reproduced on its own with the command shown beside it.
set -euo pipefail
cd "$(dirname "$0")/.."

SOURCE="${1:-corpus/fixtures}"
QUERIES="${2:-tests/fixtures/queries/basic.txt}"
BIN=./build/search

if [ ! -x "$BIN" ]; then
  echo "build first: cmake -S . -B build && cmake --build build -j" >&2
  exit 1
fi

field() { grep "^$1 " | cut -d' ' -f2-; }

echo "# Benchmark"
echo
echo "- source: \`$SOURCE\`"
echo "- date: $(date -u '+%Y-%m-%d %H:%M UTC')"
echo "- machine: $(uname -sm)$( [ "$(uname -s)" = Darwin ] && printf ', %s' "$(sysctl -n machdep.cpu.brand_string 2>/dev/null || true)" )"
echo "- cores: $(getconf _NPROCESSORS_ONLN 2>/dev/null || echo unknown)"
echo

build_one() {
  "$BIN" bench build --threads "$1" "$SOURCE"
}

BUILD1=$(build_one 1)
CORES=$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)
BUILDN=$(build_one "$CORES")
QUERY=$(  "$BIN" bench query --repeats 50 "$SOURCE" "$QUERIES")
SIZES=$(  "$BIN" index-size "$SOURCE")

docs=$(echo "$BUILD1"  | field documents)
terms=$(echo "$BUILD1" | field terms)
posts=$(echo "$BUILD1" | field postings)
bytes=$(echo "$BUILD1" | field source_bytes)
ms1=$(echo "$BUILD1"   | field build_ms)
msN=$(echo "$BUILDN"   | field build_ms)
dps=$(echo "$BUILDN"   | field documents_per_second)
mbs=$(echo "$BUILDN"   | field megabytes_per_second)
rss=$(echo "$BUILD1"   | field peak_memory_bytes)

plain=$(echo "$SIZES"   | awk '/^plain/   {print $2}')
varbyte=$(echo "$SIZES" | awk '/^varbyte/ {print $2}')
ratio=$(echo "$SIZES"   | field ratio)

p50=$(echo "$QUERY" | field p50_ms)
p95=$(echo "$QUERY" | field p95_ms)
p99=$(echo "$QUERY" | field p99_ms)
qps=$(echo "$QUERY" | field queries_per_second)
nq=$( echo "$QUERY" | field queries)

speedup=$(awk -v a="$ms1" -v b="$msN" 'BEGIN { if (b > 0) printf "%.2f", a / b; else print "n/a" }')

echo "| Measurement | Value | Reproduce with |"
echo "|---|---|---|"
echo "| Documents | $docs | \`search bench build $SOURCE\` |"
echo "| Distinct terms | $terms | same |"
echo "| Postings | $posts | same |"
echo "| Corpus bytes | $bytes | same |"
echo "| Build, 1 thread | ${ms1} ms | \`search bench build --threads 1 $SOURCE\` |"
echo "| Build, $CORES threads | ${msN} ms | \`search bench build --threads $CORES $SOURCE\` |"
echo "| Parallel speedup | ${speedup}x | the two rows above |"
echo "| Ingest rate | $dps docs/s, $mbs MB/s | \`search bench build --threads $CORES $SOURCE\` |"
echo "| Peak memory | $rss bytes | \`search bench build $SOURCE\` |"
echo "| Index, uncompressed | $plain bytes | \`search index-size $SOURCE\` |"
echo "| Index, delta + varbyte | $varbyte bytes | same |"
echo "| Compression ratio | ${ratio}x | same |"
echo "| Query latency p50 | $p50 ms | \`search bench query --repeats 50 $SOURCE $QUERIES\` |"
echo "| Query latency p95 | $p95 ms | same |"
echo "| Query latency p99 | $p99 ms | same |"
echo "| Query throughput | $qps queries/s | same |"
echo "| Queries measured | $nq | same |"
