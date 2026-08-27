#!/usr/bin/env bash
# Build the engine and run the end-to-end test suites.
#
#   ./run_tests.sh           # build, then run everything
#   ./run_tests.sh corpus    # only suites whose name contains "corpus"
#   ./run_tests.sh --list    # list the suites
set -euo pipefail
cd "$(dirname "$0")"

# A CPLUS_INCLUDE_PATH pointing at a libstdc++ installation makes clang compile
# against libstdc++ headers while still linking libc++, which fails at link time
# with a wall of undefined std::__cxx11 symbols. Drop it for this build.
unset CPLUS_INCLUDE_PATH

# Prefer a toolchain that ships a complete C++20 standard library. Set CXX
# yourself to override.
if [ -z "${CXX:-}" ]; then
  for candidate in \
    /opt/homebrew/bin/g++-15 \
    /opt/homebrew/bin/g++ \
    /opt/homebrew/opt/llvm/bin/clang++ \
    /usr/local/opt/llvm/bin/clang++ \
    /usr/bin/c++
  do
    if [ -x "$candidate" ]; then
      export CXX="$candidate"
      break
    fi
  done
fi

exec python3 tests/runner.py "$@"
