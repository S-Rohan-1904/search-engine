#!/usr/bin/env bash
# Download a Wikipedia CirrusSearch dump and build a corpus from it.
#
#   ./tools/fetch_wikipedia.sh simple             # ~640 MB, ~250k full articles
#   ./tools/fetch_wikipedia.sh simple --abstracts # same download, opening paragraphs only
#   ./tools/fetch_wikipedia.sh simple --yes       # skip the confirmation
#
# CirrusSearch dumps are what Wikipedia's own search runs on: article text with
# templates, tables and markup already stripped upstream. The abstract dumps this
# script used to fetch were discontinued by Wikimedia.
#
# The compressed dump is kept and decompressed on the fly into the importer, so
# the multi-gigabyte JSON is never written to disk. Nothing is downloaded until
# you confirm; the dump and the corpus are both gitignored.
set -euo pipefail
cd "$(dirname "$0")/.."

WIKI="${1:-simple}"
shift || true

ABSTRACTS=""
CONFIRM=""
for arg in "$@"; do
  case "$arg" in
    --abstracts) ABSTRACTS="--abstracts" ;;
    --yes) CONFIRM="--yes" ;;
    *) echo "usage: $0 [simple|en] [--abstracts] [--yes]" >&2; exit 2 ;;
  esac
done

case "$WIKI" in
  simple) SIZE="about 640 MB compressed, roughly 250,000 articles" ;;
  en)     SIZE="about 43 GB compressed, roughly 6,900,000 articles" ;;
  *)      echo "usage: $0 [simple|en] [--abstracts] [--yes]" >&2; exit 2 ;;
esac

DEST=corpus/wikipedia
BIN=./build/search

if [ ! -x "$BIN" ]; then
  echo "build first: cmake -S . -B build && cmake --build build -j" >&2
  exit 1
fi

echo "Finding the latest CirrusSearch snapshot..."
SNAPSHOT=$(curl -s --fail https://dumps.wikimedia.org/other/cirrussearch/ \
  | grep -o 'href="[0-9]\{8\}/"' | sed 's/href="//;s|/"||' | sort | tail -1)
if [ -z "$SNAPSHOT" ]; then
  echo "could not list the CirrusSearch snapshots" >&2
  exit 1
fi

NAME="${WIKI}wiki-${SNAPSHOT}-cirrussearch-content.json.gz"
URL="https://dumps.wikimedia.org/other/cirrussearch/${SNAPSHOT}/${NAME}"
GZ="$DEST/$NAME"
CORPUS="$DEST/${WIKI}wiki${ABSTRACTS:+-abstracts}.corpus"

echo
echo "About to download:"
echo "  $URL"
echo "  $SIZE"
echo
echo "It will be written to:"
echo "  $GZ  (kept compressed)"
echo "  $CORPUS  (packed corpus)"
echo

if [ "$CONFIRM" != "--yes" ]; then
  printf "Continue? [y/N] "
  read -r answer
  case "$answer" in
    y|Y|yes) ;;
    *) echo "Nothing downloaded."; exit 0 ;;
  esac
fi

mkdir -p "$DEST"

if [ -s "$GZ" ]; then
  echo "Already have $GZ, skipping the download."
else
  echo "Downloading..."
  curl --fail --location --progress-bar "$URL" -o "$GZ.partial"
  mv "$GZ.partial" "$GZ"
fi

echo "Building the corpus..."
gunzip -c "$GZ" | "$BIN" wiki-import $ABSTRACTS - "$CORPUS"

echo
echo "Done. Try:"
echo "  $BIN index-stats $CORPUS"
echo "  $BIN query --snippet $CORPUS 'solar system'"
echo "  ./tools/bench.sh $CORPUS"
