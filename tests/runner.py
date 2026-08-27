#!/usr/bin/env python3
"""End-to-end test runner.

Builds ./build/search, then runs every suite in tests/cases/ against the
binary. Suites are black box: each case is an argv, an expected stdout and an
expected exit code.
"""

import argparse
import importlib.util
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "build"
BINARY = BUILD_DIR / "search"
CASES_DIR = ROOT / "tests" / "cases"

DEFAULT_TIMEOUT = 15


# --------------------------------------------------------------------------- ui

class Style:
    def __init__(self, enabled: bool):
        self.enabled = enabled

    def _wrap(self, code: str, text: str) -> str:
        return f"\033[{code}m{text}\033[0m" if self.enabled else text

    def bold(self, t): return self._wrap("1", t)
    def dim(self, t): return self._wrap("2", t)
    def red(self, t): return self._wrap("31", t)
    def green(self, t): return self._wrap("32", t)
    def yellow(self, t): return self._wrap("33", t)


S = Style(sys.stdout.isatty())


def indent(text: str, prefix: str = "    ") -> str:
    if text == "":
        return prefix + S.dim("(empty)")
    return "\n".join(prefix + line for line in text.split("\n"))


# ---------------------------------------------------------------------- suites

def load_suite(path: Path):
    spec = importlib.util.spec_from_file_location(path.stem, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.__suite_path__ = path
    return module


def discover_suites(name_filter: str | None) -> list:
    suites = []
    for path in sorted(CASES_DIR.glob("*.py")):
        if path.name.startswith("_"):
            continue
        suite = load_suite(path)
        if name_filter and name_filter.lower() not in path.stem.lower():
            continue
        suites.append(suite)
    suites.sort(key=lambda m: (getattr(m, "ORDER", 1000), m.__suite_path__.stem))
    return suites


# ----------------------------------------------------------------------- build

def build() -> bool:
    print(S.bold("Building..."))
    configure = subprocess.run(
        ["cmake", "-S", str(ROOT), "-B", str(BUILD_DIR)],
        capture_output=True, text=True,
    )
    if configure.returncode != 0:
        print(S.red("cmake configure failed:\n"))
        print(configure.stdout + configure.stderr)
        return False

    compiled = subprocess.run(
        ["cmake", "--build", str(BUILD_DIR), "-j"],
        capture_output=True, text=True,
    )
    output = (compiled.stdout + compiled.stderr).strip()
    if compiled.returncode != 0:
        print(S.red("compilation failed:\n"))
        print(output)
        return False

    warnings = [ln for ln in output.split("\n") if "warning:" in ln]
    if warnings:
        print(S.yellow(f"  {len(warnings)} compiler warning(s)"))
        for line in warnings[:10]:
            print(S.dim("    " + line.strip()))
    print(S.green("  ok") + "\n")
    return True


# ------------------------------------------------------------------- execution

def normalize(text: str) -> str:
    """Trailing whitespace on a line, and trailing blank lines, never matter."""
    lines = [line.rstrip() for line in text.replace("\r\n", "\n").split("\n")]
    while lines and lines[-1] == "":
        lines.pop()
    return "\n".join(lines)


def render_diff(expected: str, actual: str) -> str:
    exp_lines = expected.split("\n") if expected else []
    act_lines = actual.split("\n") if actual else []
    out = []
    for i in range(max(len(exp_lines), len(act_lines))):
        e = exp_lines[i] if i < len(exp_lines) else None
        a = act_lines[i] if i < len(act_lines) else None
        if e == a:
            out.append(S.dim(f"      {e}"))
        elif e is None:
            out.append(S.red(f"    + {a}") + S.dim("   <- unexpected extra line"))
        elif a is None:
            out.append(S.green(f"    - {e}") + S.dim("   <- missing"))
        else:
            out.append(S.green(f"    - {e}"))
            out.append(S.red(f"    + {a}"))
    if len(out) > 40:
        out = out[:40] + [S.dim(f"    ... {len(out) - 40} more lines")]
    return "\n".join(out)


def run_case(case: dict) -> tuple[bool, str]:
    """Returns (passed, failure_report)."""
    argv = [str(BINARY)] + [str(a) for a in case["argv"]]
    timeout = case.get("timeout", DEFAULT_TIMEOUT)
    try:
        proc = subprocess.run(
            argv,
            input=case.get("stdin", ""),
            capture_output=True,
            text=True,
            cwd=ROOT,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired:
        return False, S.red(f"    timed out after {timeout}s")

    problems = []

    expected_code = case.get("exit_code", 0)
    if proc.returncode != expected_code:
        problems.append(
            f"    exit code: expected {S.green(str(expected_code))}, "
            f"got {S.red(str(proc.returncode))}"
        )

    if "stdout" in case:
        expected = normalize(case["stdout"])
        actual = normalize(proc.stdout)
        if expected != actual:
            problems.append("    stdout mismatch:\n" + render_diff(expected, actual))

    if case.get("stderr_not_empty") and proc.stderr.strip() == "":
        problems.append("    expected an error message on stderr, but stderr was empty")

    if not problems:
        return True, ""

    report = "\n".join(problems)
    if proc.stderr.strip():
        stderr_preview = "\n".join(proc.stderr.strip().split("\n")[:5])
        report += "\n" + S.dim("    stderr:") + "\n" + indent(stderr_preview, "      ")
    return False, report


def run_suite(suite) -> tuple[int, int]:
    """Returns (passed, total)."""
    print(S.bold(suite.NAME))
    passed = 0
    for case in suite.CASES:
        argv_preview = " ".join(
            f'"{a}"' if " " in str(a) else str(a) for a in case["argv"]
        )
        ok, report = run_case(case)
        if ok:
            passed += 1
            print(f"  {S.green('PASS')}  {case['name']}")
        else:
            print(f"  {S.red('FAIL')}  {case['name']}")
            print(S.dim(f"    $ ./build/search {argv_preview}"))
            print(report)
    print()
    return passed, len(suite.CASES)


# ------------------------------------------------------------------------ main

def main() -> int:
    parser = argparse.ArgumentParser(description="run the search engine test suites")
    parser.add_argument("suite", nargs="?",
                        help="run only suites whose name contains this substring")
    parser.add_argument("--list", action="store_true",
                        help="list the available suites and exit")
    parser.add_argument("--no-build", action="store_true",
                        help="skip the build step")
    args = parser.parse_args()

    suites = discover_suites(args.suite)
    if not suites:
        target = f" matching {args.suite!r}" if args.suite else ""
        print(S.red(f"no test suites found{target} in tests/cases/"))
        return 1

    if args.list:
        for suite in suites:
            print(f"  {suite.__suite_path__.stem:<24} {suite.NAME} "
                  f"({len(suite.CASES)} cases)")
        return 0

    if not args.no_build and not build():
        return 1

    passed = total = 0
    for suite in suites:
        suite_passed, suite_total = run_suite(suite)
        passed += suite_passed
        total += suite_total

    if passed == total:
        print(S.green(S.bold(f"All {total} checks passing.")))
        return 0

    print(S.red(S.bold(f"{passed}/{total} checks passing.")))
    return 1


if __name__ == "__main__":
    sys.exit(main())
