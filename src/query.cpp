#include "query.hpp"

#include <cctype>
#include <utility>

namespace {

bool is_query_space(unsigned char c) {
    return std::isspace(c) != 0;
}

bool is_delimiter(unsigned char c) {
    return is_query_space(c) || c == '(' || c == ')' || c == '"';
}

}

bool lex_query(std::string_view query, std::vector<QueryToken>& out, std::string& error) {
    out.clear();
    error.clear();

    std::size_t i = 0;
    while (i < query.size()) {
        const unsigned char c = static_cast<unsigned char>(query[i]);

        if (is_query_space(c)) {
            i++;
            continue;
        }

        if (c == '(') {
            out.push_back(QueryToken{QueryTokenKind::LParen, ""});
            i++;
            continue;
        }

        if (c == ')') {
            out.push_back(QueryToken{QueryTokenKind::RParen, ""});
            i++;
            continue;
        }

        if (c == '"') {
            const std::size_t start = i + 1;
            const std::size_t close = query.find('"', start);
            if (close == std::string_view::npos) {
                error = "unterminated quote";
                out.clear();
                return false;
            }
            out.push_back(QueryToken{QueryTokenKind::Phrase,
                                     std::string(query.substr(start, close - start))});
            i = close + 1;
            continue;
        }

        const std::size_t start = i;
        while (i < query.size() && !is_delimiter(static_cast<unsigned char>(query[i]))) {
            i++;
        }

        const std::string_view word = query.substr(start, i - start);
        if (word == "AND") {
            out.push_back(QueryToken{QueryTokenKind::And, ""});
        } else if (word == "OR") {
            out.push_back(QueryToken{QueryTokenKind::Or, ""});
        } else if (word == "NOT") {
            out.push_back(QueryToken{QueryTokenKind::Not, ""});
        } else {
            out.push_back(QueryToken{QueryTokenKind::Term, std::string(word)});
        }
    }

    return true;
}

namespace {

class Parser {
public:
    explicit Parser(const std::vector<QueryToken>& tokens) : tokens_(tokens) {}

    std::unique_ptr<QueryNode> parse() {
        std::unique_ptr<QueryNode> root = parse_or();
        if (root && position_ != tokens_.size()) {
            error_ = "unexpected token after end of query";
            return nullptr;
        }
        return root;
    }

    const std::string& error() const { return error_; }

private:
    static std::unique_ptr<QueryNode> make(QueryNodeKind kind, std::string text) {
        auto node = std::make_unique<QueryNode>();
        node->kind = kind;
        node->text = std::move(text);
        return node;
    }

    static std::unique_ptr<QueryNode> combine(QueryNodeKind kind,
                                              std::vector<std::unique_ptr<QueryNode>> children) {
        if (children.size() == 1) {
            return std::move(children.front());
        }
        auto node = make(kind, "");
        node->children = std::move(children);
        return node;
    }

    bool at_end() const { return position_ >= tokens_.size(); }

    bool peek_is(QueryTokenKind kind) const {
        return !at_end() && tokens_[position_].kind == kind;
    }

    bool starts_operand() const {
        return peek_is(QueryTokenKind::Term) || peek_is(QueryTokenKind::Phrase) ||
               peek_is(QueryTokenKind::LParen) || peek_is(QueryTokenKind::Not);
    }

    std::unique_ptr<QueryNode> parse_or() {
        std::unique_ptr<QueryNode> first = parse_and();
        if (!first) {
            return nullptr;
        }

        std::vector<std::unique_ptr<QueryNode>> children;
        children.push_back(std::move(first));

        while (peek_is(QueryTokenKind::Or)) {
            position_++;
            std::unique_ptr<QueryNode> next = parse_and();
            if (!next) {
                return nullptr;
            }
            children.push_back(std::move(next));
        }

        return combine(QueryNodeKind::Or, std::move(children));
    }

    std::unique_ptr<QueryNode> parse_and() {
        std::unique_ptr<QueryNode> first = parse_unary();
        if (!first) {
            return nullptr;
        }

        std::vector<std::unique_ptr<QueryNode>> children;
        children.push_back(std::move(first));

        while (true) {
            if (peek_is(QueryTokenKind::And)) {
                position_++;
            } else if (!starts_operand()) {
                break;
            }

            std::unique_ptr<QueryNode> next = parse_unary();
            if (!next) {
                return nullptr;
            }
            children.push_back(std::move(next));
        }

        return combine(QueryNodeKind::And, std::move(children));
    }

    std::unique_ptr<QueryNode> parse_unary() {
        if (peek_is(QueryTokenKind::Not)) {
            position_++;
            std::unique_ptr<QueryNode> operand = parse_unary();
            if (!operand) {
                return nullptr;
            }
            auto node = make(QueryNodeKind::Not, "");
            node->children.push_back(std::move(operand));
            return node;
        }

        return parse_primary();
    }

    std::unique_ptr<QueryNode> parse_primary() {
        if (at_end()) {
            error_ = "unexpected end of query";
            return nullptr;
        }

        const QueryToken& token = tokens_[position_];
        switch (token.kind) {
        case QueryTokenKind::Term:
            position_++;
            return make(QueryNodeKind::Term, token.text);

        case QueryTokenKind::Phrase:
            position_++;
            return make(QueryNodeKind::Phrase, token.text);

        case QueryTokenKind::LParen: {
            position_++;
            std::unique_ptr<QueryNode> inner = parse_or();
            if (!inner) {
                return nullptr;
            }
            if (!peek_is(QueryTokenKind::RParen)) {
                error_ = "missing closing parenthesis";
                return nullptr;
            }
            position_++;
            return inner;
        }

        case QueryTokenKind::RParen:
            error_ = "unexpected closing parenthesis";
            return nullptr;

        default:
            error_ = "unexpected operator";
            return nullptr;
        }
    }

    const std::vector<QueryToken>& tokens_;
    std::size_t position_ = 0;
    std::string error_;
};

const char* node_name(QueryNodeKind kind) {
    switch (kind) {
    case QueryNodeKind::Term:   return "term";
    case QueryNodeKind::Phrase: return "phrase";
    case QueryNodeKind::And:    return "and";
    case QueryNodeKind::Or:     return "or";
    case QueryNodeKind::Not:    return "not";
    }
    return "?";
}

}

ParseResult parse_query(std::string_view query) {
    std::vector<QueryToken> tokens;
    std::string error;
    if (!lex_query(query, tokens, error)) {
        return ParseResult{nullptr, error};
    }
    if (tokens.empty()) {
        return ParseResult{nullptr, "empty query"};
    }

    Parser parser(tokens);
    std::unique_ptr<QueryNode> root = parser.parse();
    if (!root) {
        return ParseResult{nullptr, parser.error()};
    }

    return ParseResult{std::move(root), ""};
}

std::string to_sexpr(const QueryNode& node) {
    std::string out = "(";
    out += node_name(node.kind);

    if (!node.text.empty()) {
        out += " ";
        out += node.text;
    }

    for (const std::unique_ptr<QueryNode>& child : node.children) {
        out += " ";
        out += to_sexpr(*child);
    }

    out += ")";
    return out;
}
