#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "analyzer.hpp"
#include "corpus.hpp"
#include "document.hpp"
#include "evaluator.hpp"
#include "index.hpp"
#include "normalize.hpp"
#include "query.hpp"
#include "stemmer.hpp"
#include "stopwords.hpp"
#include "tokenizer.hpp"

namespace {

int usage() {
    std::cerr << "usage: search <command> [args...]\n"
                 "\n"
                 "commands:\n"
                 "  docs <corpus_dir>            list the document ids in a corpus\n"
                 "  show <corpus_dir> <doc_id>   print one document's id, title and body\n"
                 "  tokenize <text>              split text into tokens\n"
                 "  normalize <token>            reduce one token to its indexed term\n"
                 "  terms [--drop-stopwords] <text>\n"
                 "                               tokenize, then normalize\n"
                 "  stopword <word>              is this word a stopword?\n"
                 "  stem-info <word>             Porter consonant pattern and measure\n"
                 "  stem-step <step> <word>      apply one Porter step\n"
                 "                               (1a, 1b, 1c, 2, 3, 4, 5a, 5b)\n"
                 "  stem <word>                  reduce a word to its Porter stem\n"
                 "  analyze <text>               run the full analysis chain\n"
                 "  analyze-doc <corpus_dir> <doc_id>\n"
                 "                               analyze one document from disk\n"
                 "  index-stats <corpus_dir>     document, term and posting counts\n"
                 "  index-terms <corpus_dir>     every indexed term with its document frequency\n"
                 "  postings <corpus_dir> <word>  documents containing a word\n"
                 "  tf <corpus_dir> <word>       per-document counts for a word\n"
                 "  positions <corpus_dir> <word> token positions of a word per document\n"
                 "  lex <query>                  split a query into syntactic tokens\n"
                 "  parse <query>                parse a query into a boolean tree\n"
                 "  and <corpus_dir> <word>...   documents containing every word\n"
                 "  or <corpus_dir> <word>...    documents containing any of the words\n"
                 "  andnot <corpus_dir> <word> <word>\n"
                 "                               documents containing the first but not the second\n"
                 "  phrase <corpus_dir> <text>   documents containing the words consecutively\n"
                 "  match <corpus_dir> <query>   documents matching a boolean query\n";
    return 2;
}

// Reports a corpus directory that does not exist. Returns the exit code.
int reject_missing_corpus(const std::filesystem::path& corpus_dir) {
    std::cerr << "search: not a directory: " << corpus_dir.string() << "\n";
    return 1;
}

int cmd_docs(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    const std::filesystem::path corpus_dir = args[0];
    std::error_code ec;
    if (!std::filesystem::is_directory(corpus_dir, ec)) {
        return reject_missing_corpus(corpus_dir);
    }

    for (const std::string& id : list_document_ids(corpus_dir)) {
        std::cout << id << "\n";
    }
    return 0;
}

int cmd_show(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return usage();
    }

    const std::filesystem::path corpus_dir = args[0];
    const std::string& id = args[1];

    std::error_code ec;
    if (!std::filesystem::is_directory(corpus_dir, ec)) {
        return reject_missing_corpus(corpus_dir);
    }
    if (!std::filesystem::is_regular_file(corpus_dir / (id + ".txt"), ec)) {
        std::cerr << "search: no such document: " << id << "\n";
        return 1;
    }

    const std::optional<Document> doc = read_document(corpus_dir, id);
    if (!doc.has_value()) {
        std::cerr << "search: malformed document: " << id << "\n";
        return 1;
    }

    std::cout << "id: " << doc->id << "\n"
              << "title: " << doc->title << "\n"
              << "\n"
              << doc->body << "\n";
    return 0;
}

int cmd_tokenize(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    for (const Token& token : tokenize(args[0])) {
        std::cout << token.position << " " << token.offset << " " << token.text << "\n";
    }
    return 0;
}

int cmd_normalize(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    std::cout << normalize(args[0]) << "\n";
    return 0;
}

int cmd_terms(const std::vector<std::string>& args) {
    std::vector<std::string> rest = args;
    bool drop_stopwords = false;
    if (!rest.empty() && rest.front() == "--drop-stopwords") {
        drop_stopwords = true;
        rest.erase(rest.begin());
    }
    if (rest.size() != 1) {
        return usage();
    }

    std::vector<Token> tokens = normalize_tokens(tokenize(rest[0]));
    if (drop_stopwords) {
        tokens = remove_stopwords(std::move(tokens));
    }

    for (const Token& token : tokens) {
        std::cout << token.position << " " << token.offset << " " << token.text << "\n";
    }
    return 0;
}

int cmd_stopword(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    std::cout << (is_stopword(normalize(args[0])) ? "yes" : "no") << "\n";
    return 0;
}

void print_terms(const std::vector<Token>& tokens) {
    for (const Token& token : tokens) {
        std::cout << token.position << " " << token.offset << " " << token.text << "\n";
    }
}

void print_document_ids(const InvertedIndex& index, const std::vector<std::size_t>& matches) {
    const std::vector<std::string>& ids = index.document_ids();
    for (const std::size_t doc_id : matches) {
        std::cout << ids[doc_id] << "\n";
    }
}

bool literal_doc_ids(const InvertedIndex& index, const std::string& word,
                     std::vector<std::size_t>& out) {
    const std::vector<Token> terms = analyze(word);
    if (terms.size() > 1) {
        std::cerr << "search: not a single term: " << word << "\n";
        return false;
    }

    out.clear();
    if (!terms.empty()) {
        for (const Posting& posting : index.postings(terms.front().text)) {
            out.push_back(posting.doc_id);
        }
    }
    return true;
}

template <typename Combine>
int run_set_query(const std::vector<std::string>& args, std::size_t min_words, Combine combine) {
    if (args.size() < min_words + 1) {
        return usage();
    }

    const std::filesystem::path corpus_dir = args[0];
    std::error_code ec;
    if (!std::filesystem::is_directory(corpus_dir, ec)) {
        return reject_missing_corpus(corpus_dir);
    }

    const InvertedIndex index = build_index(corpus_dir);

    std::vector<std::size_t> result;
    if (!literal_doc_ids(index, args[1], result)) {
        return 1;
    }

    for (std::size_t i = 2; i < args.size(); i++) {
        std::vector<std::size_t> next;
        if (!literal_doc_ids(index, args[i], next)) {
            return 1;
        }
        result = combine(result, next);
    }

    print_document_ids(index, result);
    return 0;
}

int cmd_and(const std::vector<std::string>& args) {
    return run_set_query(args, 2, intersect);
}

int cmd_or(const std::vector<std::string>& args) {
    return run_set_query(args, 2, unite);
}

int cmd_andnot(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return usage();
    }
    return run_set_query(args, 2, subtract);
}

int run_parsed_query(const std::vector<std::string>& args, const std::string& query) {
    const std::filesystem::path corpus_dir = args[0];
    std::error_code ec;
    if (!std::filesystem::is_directory(corpus_dir, ec)) {
        return reject_missing_corpus(corpus_dir);
    }

    const ParseResult parsed = parse_query(query);
    if (!parsed.root) {
        std::cerr << "search: " << parsed.error << "\n";
        return 1;
    }

    const InvertedIndex index = build_index(corpus_dir);
    print_document_ids(index, evaluate(*parsed.root, index));
    return 0;
}

int cmd_match(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return usage();
    }
    return run_parsed_query(args, args[1]);
}

int cmd_phrase(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return usage();
    }
    if (args[1].find('"') != std::string::npos) {
        std::cerr << "search: a phrase may not contain a quote\n";
        return 1;
    }
    return run_parsed_query(args, "\"" + args[1] + "\"");
}

int cmd_lex(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    std::vector<QueryToken> tokens;
    std::string error;
    if (!lex_query(args[0], tokens, error)) {
        std::cerr << "search: " << error << "\n";
        return 1;
    }

    for (const QueryToken& token : tokens) {
        switch (token.kind) {
        case QueryTokenKind::Term:   std::cout << "term " << token.text << "\n"; break;
        case QueryTokenKind::Phrase: std::cout << "phrase " << token.text << "\n"; break;
        case QueryTokenKind::And:    std::cout << "and\n"; break;
        case QueryTokenKind::Or:     std::cout << "or\n"; break;
        case QueryTokenKind::Not:    std::cout << "not\n"; break;
        case QueryTokenKind::LParen: std::cout << "lparen\n"; break;
        case QueryTokenKind::RParen: std::cout << "rparen\n"; break;
        }
    }
    return 0;
}

int cmd_parse(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    const ParseResult result = parse_query(args[0]);
    if (!result.root) {
        std::cerr << "search: " << result.error << "\n";
        return 1;
    }

    std::cout << to_sexpr(*result.root) << "\n";
    return 0;
}

template <typename Emit>
int run_term_query(const std::vector<std::string>& args, Emit emit) {
    if (args.size() != 2) {
        return usage();
    }

    const std::filesystem::path corpus_dir = args[0];
    std::error_code ec;
    if (!std::filesystem::is_directory(corpus_dir, ec)) {
        return reject_missing_corpus(corpus_dir);
    }

    const std::vector<Token> query = analyze(args[1]);
    if (query.size() > 1) {
        std::cerr << "search: not a single term: " << args[1] << "\n";
        return 1;
    }
    if (query.empty()) {
        return 0;
    }

    const InvertedIndex index = build_index(corpus_dir);
    const std::vector<std::string>& ids = index.document_ids();
    for (const Posting& posting : index.postings(query.front().text)) {
        emit(ids[posting.doc_id], posting);
    }
    return 0;
}

int cmd_postings(const std::vector<std::string>& args) {
    return run_term_query(args, [](const std::string& id, const Posting&) {
        std::cout << id << "\n";
    });
}

int cmd_tf(const std::vector<std::string>& args) {
    return run_term_query(args, [](const std::string& id, const Posting& posting) {
        std::cout << id << " " << posting.frequency << "\n";
    });
}

int cmd_positions(const std::vector<std::string>& args) {
    return run_term_query(args, [](const std::string& id, const Posting& posting) {
        std::cout << id;
        for (const std::size_t position : posting.positions) {
            std::cout << " " << position;
        }
        std::cout << "\n";
    });
}

int cmd_index_stats(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    const std::filesystem::path corpus_dir = args[0];
    std::error_code ec;
    if (!std::filesystem::is_directory(corpus_dir, ec)) {
        return reject_missing_corpus(corpus_dir);
    }

    const InvertedIndex index = build_index(corpus_dir);
    std::cout << "documents: " << index.document_count() << "\n"
              << "terms: " << index.term_count() << "\n"
              << "postings: " << index.posting_count() << "\n";
    return 0;
}

int cmd_index_terms(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    const std::filesystem::path corpus_dir = args[0];
    std::error_code ec;
    if (!std::filesystem::is_directory(corpus_dir, ec)) {
        return reject_missing_corpus(corpus_dir);
    }

    const InvertedIndex index = build_index(corpus_dir);
    for (const std::string& term : index.terms()) {
        std::cout << term << " " << index.document_frequency(term) << "\n";
    }
    return 0;
}

int cmd_analyze(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    print_terms(analyze(args[0]));
    return 0;
}

int cmd_analyze_doc(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return usage();
    }

    const std::filesystem::path corpus_dir = args[0];
    const std::string& id = args[1];

    std::error_code ec;
    if (!std::filesystem::is_directory(corpus_dir, ec)) {
        return reject_missing_corpus(corpus_dir);
    }

    const std::optional<Document> doc = read_document(corpus_dir, id);
    if (!doc.has_value()) {
        std::cerr << "search: no such document: " << id << "\n";
        return 1;
    }

    print_terms(analyze_document(*doc));
    return 0;
}

int cmd_stem(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    std::cout << porter::stem(args[0]) << "\n";
    return 0;
}

int cmd_stem_step(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return usage();
    }

    const std::string& step = args[0];
    const std::string& word = args[1];

    if (step == "1a") {
        std::cout << porter::step_1a_plurals(word) << "\n";
        return 0;
    }
    if (step == "1b") {
        std::cout << porter::step_1b_verb_endings(word) << "\n";
        return 0;
    }
    if (step == "1c") {
        std::cout << porter::step_1c_y_to_i(word) << "\n";
        return 0;
    }
    if (step == "2") {
        std::cout << porter::step_2_double_suffixes(word) << "\n";
        return 0;
    }
    if (step == "3") {
        std::cout << porter::step_3_derived_suffixes(word) << "\n";
        return 0;
    }
    if (step == "4") {
        std::cout << porter::step_4_residual_suffixes(word) << "\n";
        return 0;
    }
    if (step == "5a") {
        std::cout << porter::step_5a_final_e(word) << "\n";
        return 0;
    }
    if (step == "5b") {
        std::cout << porter::step_5b_double_l(word) << "\n";
        return 0;
    }

    std::cerr << "search: unknown stemmer step: " << step << "\n";
    return 1;
}

int cmd_stem_info(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    const std::string& word = args[0];

    std::string pattern;
    pattern.reserve(word.size());
    for (std::size_t i = 0; i < word.size(); ++i) {
        pattern.push_back(porter::is_consonant(word, i) ? 'C' : 'V');
    }

    std::cout << "pattern: " << pattern << "\n"
              << "measure: " << porter::measure(word) << "\n"
              << "vowel: " << (porter::contains_vowel(word) ? "yes" : "no") << "\n"
              << "double: " << (porter::ends_with_double_consonant(word) ? "yes" : "no") << "\n"
              << "cvc: " << (porter::ends_cvc(word) ? "yes" : "no") << "\n";
    return 0;
}

} 

int main(int argc, char** argv) {
    const std::vector<std::string> args(argv + 1, argv + argc);
    if (args.empty()) {
        return usage();
    }

    const std::string command = args[0];
    const std::vector<std::string> rest(args.begin() + 1, args.end());

    if (command == "docs") {
        return cmd_docs(rest);
    }
    if (command == "show") {
        return cmd_show(rest);
    }
    if (command == "tokenize") {
        return cmd_tokenize(rest);
    }
    if (command == "normalize") {
        return cmd_normalize(rest);
    }
    if (command == "terms") {
        return cmd_terms(rest);
    }
    if (command == "stopword") {
        return cmd_stopword(rest);
    }
    if (command == "stem-info") {
        return cmd_stem_info(rest);
    }
    if (command == "stem") {
        return cmd_stem(rest);
    }
    if (command == "analyze") {
        return cmd_analyze(rest);
    }
    if (command == "index-stats") {
        return cmd_index_stats(rest);
    }
    if (command == "index-terms") {
        return cmd_index_terms(rest);
    }
    if (command == "postings") {
        return cmd_postings(rest);
    }
    if (command == "tf") {
        return cmd_tf(rest);
    }
    if (command == "positions") {
        return cmd_positions(rest);
    }
    if (command == "lex") {
        return cmd_lex(rest);
    }
    if (command == "parse") {
        return cmd_parse(rest);
    }
    if (command == "and") {
        return cmd_and(rest);
    }
    if (command == "or") {
        return cmd_or(rest);
    }
    if (command == "andnot") {
        return cmd_andnot(rest);
    }
    if (command == "phrase") {
        return cmd_phrase(rest);
    }
    if (command == "match") {
        return cmd_match(rest);
    }
    if (command == "analyze-doc") {
        return cmd_analyze_doc(rest);
    }
    if (command == "stem-step") {
        return cmd_stem_step(rest);
    }

    std::cerr << "search: unknown command: " << command << "\n";
    return usage();
}
