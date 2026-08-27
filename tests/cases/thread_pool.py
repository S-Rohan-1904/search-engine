"""`search pool-sum <threads> <n>` -- tasks run on a pool and results are collected in order."""

NAME = "Thread pool"
ORDER = 310

CASES = [
    {
        "name": "one worker runs every task",
        "argv": ["pool-sum", "1", "1000"],
        "stdout": "threads 1\ntasks 1000\nsum 332833500",
    },
    {
        "name": "four workers reach the same total",
        "argv": ["pool-sum", "4", "1000"],
        "stdout": "threads 4\ntasks 1000\nsum 332833500",
    },
    {
        "name": "so do sixteen",
        "argv": ["pool-sum", "16", "1000"],
        "stdout": "threads 16\ntasks 1000\nsum 332833500",
    },
    {
        "name": "more workers than tasks is fine",
        "argv": ["pool-sum", "8", "3"],
        "stdout": "threads 8\ntasks 3\nsum 5",
    },
    {
        "name": "no tasks at all is fine",
        "argv": ["pool-sum", "4", "0"],
        "stdout": "threads 4\ntasks 0\nsum 0",
    },
    {
        "name": "a single task",
        "argv": ["pool-sum", "4", "1"],
        "stdout": "threads 4\ntasks 1\nsum 0",
    },
    {
        "name": "many tasks on few workers",
        "argv": ["pool-sum", "2", "5000"],
        "stdout": "threads 2\ntasks 5000\nsum 41654167500",
    },
    {
        "name": "the same many tasks on many workers",
        "argv": ["pool-sum", "16", "5000"],
        "stdout": "threads 16\ntasks 5000\nsum 41654167500",
    },
    {
        "name": "zero workers is rejected",
        "argv": ["pool-sum", "0", "10"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a non-numeric worker count is rejected",
        "argv": ["pool-sum", "x", "10"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a negative task count is rejected",
        "argv": ["pool-sum", "4", "-1"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "pool-sum with one argument is rejected",
        "argv": ["pool-sum", "4"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
