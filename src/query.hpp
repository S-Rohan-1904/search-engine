#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <vector>

// What a piece of query syntax is.
enum class QueryTokenKind {
    Term,    // a bare word
    Phrase,  // the contents of a "quoted string"
    And,
    Or,
    Not,
    LParen,
    RParen,
};

struct QueryToken {
    QueryTokenKind kind;
    std::string text;  // the word or phrase; empty for operators and parentheses
};

// Splits a query into syntactic tokens.
//
// Operators are the bare uppercase words AND, OR and NOT. Lowercase "and" is a
// term, not an operator, which keeps a query like `cats and dogs` meaning what
// a user typed rather than what the grammar would prefer.
//
// Parentheses are single characters and need no surrounding space. A double
// quote opens a phrase that runs to the next double quote; the contents are
// kept verbatim, including any AND or OR inside them.
//
// Nothing here is analyzed. The tokens carry the user's original spelling, and
// running them through the analyzer is the evaluator's job.
//
// Returns false and leaves error set if the query is malformed, which today
// means only an unterminated quote.
bool lex_query(std::string_view query, std::vector<QueryToken>& out, std::string& error);

// What a node in the parsed query tree is.
enum class QueryNodeKind {
    Term,
    Phrase,
    And,
    Or,
    Not,
};

// One node of the query tree. Term and Phrase are leaves and carry text; And
// and Or are n-ary and carry two or more children; Not carries exactly one.
//
// And and Or are flattened rather than nested pairwise, so `a b c` is a single
// And with three children. Evaluation in Phase 3 can then intersect the
// shortest postings list first, which a pairwise tree would not allow.
struct QueryNode {
    QueryNodeKind kind;
    std::string text;
    std::vector<std::unique_ptr<QueryNode>> children;
};

// A parse either produces a tree or an explanation.
struct ParseResult {
    std::unique_ptr<QueryNode> root;  // null when the parse failed
    std::string error;                // empty when it succeeded
};

// Lexes and parses a query into a tree.
//
// Precedence, tightest first: NOT, then AND, then OR. So
//
//     cat OR dog AND bird     is    cat OR (dog AND bird)
//     NOT cat OR dog          is    (NOT cat) OR dog
//
// Parentheses override it. Two operands with no operator between them are
// joined by AND, so `cat dog` means both words, which is what a searcher
// typing into a box expects.
//
// Nesting is capped at 256 levels. The parser is recursive descent and the
// tree is walked recursively everywhere downstream, so an unbounded depth
// would let a pathological query exhaust the stack.
ParseResult parse_query(std::string_view query);

// Renders a tree as an S-expression: (or (term cat) (and (term dog) (term bird)))
std::string to_sexpr(const QueryNode& node);
