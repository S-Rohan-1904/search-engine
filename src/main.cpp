#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "analyzer.hpp"
#include "corpus.hpp"
#include "document.hpp"
#include "normalize.hpp"
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
                 "                               analyze one document from disk\n";
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
    if (command == "analyze-doc") {
        return cmd_analyze_doc(rest);
    }
    if (command == "stem-step") {
        return cmd_stem_step(rest);
    }

    std::cerr << "search: unknown command: " << command << "\n";
    return usage();
}
