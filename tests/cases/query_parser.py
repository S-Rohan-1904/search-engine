"""`search parse <query>` -- the boolean query parser."""

NAME = "Query parser"
ORDER = 160

CASES = [
    {
        "name": "a single word parses to a bare term",
        "argv": ["parse", "cat"],
        "stdout": "(term cat)",
    },
    {
        "name": "two words with no operator are joined by AND",
        "argv": ["parse", "cat dog"],
        "stdout": "(and (term cat) (term dog))",
    },
    {
        "name": "an explicit AND parses the same way",
        "argv": ["parse", "cat AND dog"],
        "stdout": "(and (term cat) (term dog))",
    },
    {
        "name": "AND binds tighter than OR",
        "argv": ["parse", "cat OR dog AND bird"],
        "stdout": "(or (term cat) (and (term dog) (term bird)))",
    },
    {
        "name": "NOT binds tighter than OR",
        "argv": ["parse", "NOT cat OR dog"],
        "stdout": "(or (not (term cat)) (term dog))",
    },
    {
        "name": "NOT binds tighter than AND",
        "argv": ["parse", "NOT cat AND dog"],
        "stdout": "(and (not (term cat)) (term dog))",
    },
    {
        "name": "parentheses override precedence",
        "argv": ["parse", "(cat OR dog) AND bird"],
        "stdout": "(and (or (term cat) (term dog)) (term bird))",
    },
    {
        "name": "AND is flattened rather than nested pairwise",
        "argv": ["parse", "a b c"],
        "stdout": "(and (term a) (term b) (term c))",
    },
    {
        "name": "OR is flattened too",
        "argv": ["parse", "a OR b OR c"],
        "stdout": "(or (term a) (term b) (term c))",
    },
    {
        "name": "NOT stacks",
        "argv": ["parse", "NOT NOT cat"],
        "stdout": "(not (not (term cat)))",
    },
    {
        "name": "a phrase is a leaf like a term",
        "argv": ["parse", "\"tiny document\" AND cat"],
        "stdout": "(and (phrase tiny document) (term cat))",
    },
    {
        "name": "operators inside a phrase are literal text",
        "argv": ["parse", "\"cat AND dog\""],
        "stdout": "(phrase cat AND dog)",
    },
    {
        "name": "nesting combines all three operators",
        "argv": ["parse", "cat AND (dog OR NOT bird)"],
        "stdout": "(and (term cat) (or (term dog) (not (term bird))))",
    },
    {
        "name": "redundant parentheses collapse",
        "argv": ["parse", "((cat))"],
        "stdout": "(term cat)",
    },
    {
        "name": "terms keep their original spelling",
        "argv": ["parse", "Cats RUNNING"],
        "stdout": "(and (term Cats) (term RUNNING))",
    },
    {
        "name": "an empty query is rejected",
        "argv": ["parse", ""],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a trailing operator is rejected",
        "argv": ["parse", "cat AND"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an unclosed parenthesis is rejected",
        "argv": ["parse", "(cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a stray closing parenthesis is rejected",
        "argv": ["parse", "cat)"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a leading operator is rejected",
        "argv": ["parse", "AND cat"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "a lexer error is reported by the parser",
        "argv": ["parse", "cat \"dog"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "an empty parenthesis group is rejected",
        "argv": ["parse", "()"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "parse with no query is rejected",
        "argv": ["parse"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
