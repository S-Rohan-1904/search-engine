"""`search lex <query>` -- the query lexer."""

NAME = "Query lexer"
ORDER = 150

CASES = [
    {
        "name": "a bare word is a term",
        "argv": ["lex", "cat"],
        "stdout": "term cat",
    },
    {
        "name": "whitespace separates terms",
        "argv": ["lex", "cat dog bird"],
        "stdout": "term cat\nterm dog\nterm bird",
    },
    {
        "name": "uppercase AND OR NOT are operators",
        "argv": ["lex", "cat AND dog OR NOT bird"],
        "stdout": "term cat\nand\nterm dog\nor\nnot\nterm bird",
    },
    {
        "name": "lowercase and is a term, not an operator",
        "argv": ["lex", "cats and dogs"],
        "stdout": "term cats\nterm and\nterm dogs",
    },
    {
        "name": "mixed case is a term too",
        "argv": ["lex", "cat And dog"],
        "stdout": "term cat\nterm And\nterm dog",
    },
    {
        "name": "parentheses are their own tokens",
        "argv": ["lex", "(cat OR dog) AND bird"],
        "stdout": "lparen\nterm cat\nor\nterm dog\nrparen\nand\nterm bird",
    },
    {
        "name": "parentheses need no surrounding whitespace",
        "argv": ["lex", "(cat)"],
        "stdout": "lparen\nterm cat\nrparen",
    },
    {
        "name": "a quoted string is one phrase token",
        "argv": ["lex", "\"tiny document\""],
        "stdout": "phrase tiny document",
    },
    {
        "name": "operators inside a phrase stay literal",
        "argv": ["lex", "\"cat AND dog\""],
        "stdout": "phrase cat AND dog",
    },
    {
        "name": "an empty phrase is legal",
        "argv": ["lex", "\"\""],
        "stdout": "phrase",
    },
    {
        "name": "terms keep their original spelling and case",
        "argv": ["lex", "Cats RUNNING don't"],
        "stdout": "term Cats\nterm RUNNING\nterm don't",
    },
    {
        "name": "an empty query lexes to nothing",
        "argv": ["lex", ""],
        "stdout": "",
    },
    {
        "name": "whitespace only lexes to nothing",
        "argv": ["lex", "   \t  "],
        "stdout": "",
    },
    {
        "name": "a phrase can sit next to parentheses",
        "argv": ["lex", "(\"a b\" OR c)"],
        "stdout": "lparen\nphrase a b\nor\nterm c\nrparen",
    },
    {
        "name": "an unterminated quote is rejected",
        "argv": ["lex", "cat \"dog"],
        "exit_code": 1,
        "stderr_not_empty": True,
    },
    {
        "name": "lex with no query is rejected",
        "argv": ["lex"],
        "exit_code": 2,
        "stderr_not_empty": True,
    },
]
