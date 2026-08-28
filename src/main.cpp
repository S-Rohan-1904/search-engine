#include <filesystem>
#include <unistd.h>
#include <iomanip>
#include <iostream>
#include <chrono>
#include <fstream>
#include <iterator>
#include <sstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "analyzer.hpp"
#include "complete.hpp"
#include "corpus.hpp"
#include "crawler.hpp"
#include "document.hpp"
#include "edit_distance.hpp"
#include "fetcher.hpp"
#include "html.hpp"
#include "evaluator.hpp"
#include "index.hpp"
#include "index_io.hpp"
#include "normalize.hpp"
#include "pagerank.hpp"
#include "query.hpp"
#include "robots.hpp"
#include "snippet.hpp"
#include "suggest.hpp"
#include "ranking.hpp"
#include "results.hpp"
#include "thread_pool.hpp"
#include "url.hpp"
#include "thread_pool.hpp"
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
                 "  match <corpus_dir> <query>   documents matching a boolean query\n"
                 "  query [options] <source> <query>\n"
                 "                               ranked results for a boolean query\n"
                 "      --limit <n>              how many results (default 10)\n"
                 "      --scorer bm25|tfidf      which ranking function\n"
                 "      --snippet                show an excerpt under each result\n"
                 "      --max-chars <n>          how long an excerpt may be\n"
                 "      --tsv                    id and score, tab separated\n"
                 "  repl [options] <source>      read queries until end of input\n"
                 "  lengths <corpus_dir>         indexed length of each document\n"
                 "  tfidf [--threads <n>] <source> <word>...\n"
                 "                               rank documents by TF-IDF\n"
                 "  bm25 [--threads <n>] <source> <word>...\n"
                 "                               rank documents by BM25\n"
                 "  top [--bm25] [--threads <n>] <source> <k> <word>...\n"
                 "                               the k highest scoring documents\n"
                 "  index-write [--encoding <name>] [--threads <n>] <source> <file>\n"
                 "                               save an index to disk\n"
                 "  index-update [--encoding <name>] <corpus_dir> <file>\n"
                 "                               re-index only what changed\n"
                 "  index-size <source>          encoded size under each encoding\n"
                 "  pool-sum <threads> <n>       sum of squares below n, on a thread pool\n"
                 "  snippet [--max-chars <n>] <corpus_dir> <doc_id> <word>...\n"
                 "                               an excerpt with the matches marked\n"
                 "  edit-distance <a> <b>        Levenshtein distance between two words\n"
                 "  suggest [--max-distance <n>] [--limit <n>] <source> <word>\n"
                 "                               dictionary terms near a misspelling\n"
                 "  complete [--limit <n>] <source> <prefix>\n"
                 "                               dictionary terms beginning with a prefix\n"
                 "  pagerank [--iterations <n>] [--damping <d>] <links_file>\n"
                 "                               PageRank over a crawl's link graph\n"
                 "  url <url>                    normalize a url\n"
                 "  url-resolve <base> <ref>     resolve a link against a base url\n"
                 "  robots <file> <agent> <path> is this path allowed?\n"
                 "  crawl-delay <file> <agent> <floor_ms>\n"
                 "                               milliseconds to wait between requests\n"
                 "  html-text <file>             title and visible text of an html file\n"
                 "  html-links <base> <file>     links from an html file, resolved\n"
                 "  crawl [options] <seed_url>   crawl from a seed url\n"
                 "      --delay <ms>             floor between requests to one host\n"
                 "      --mirror <dir>           fetch from a mirrored site on disk\n"
                 "      --out <dir>              write the pages as a corpus\n"
                 "      --max-pages <n>          stop after n pages\n"
                 "      --max-depth <n>          follow links at most n deep\n"
                 "      --user-agent <name>      the name robots.txt is matched against\n"
                 "      --all-hosts              follow links off the seed host\n"
                 "\n"
                 "<source> is a corpus directory or a saved index file.\n";
    return 2;
}

// Reports a corpus directory that does not exist. Returns the exit code.
int reject_missing_corpus(const std::filesystem::path& corpus_dir) {
    std::cerr << "search: not a directory: " << corpus_dir.string() << "\n";
    return 1;
}

bool parse_count(const std::string& text, std::size_t& out) {
    try {
        std::size_t consumed = 0;
        const long long parsed = std::stoll(text, &consumed);
        if (consumed != text.size() || parsed < 0) {
            return false;
        }
        out = static_cast<std::size_t>(parsed);
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool take_threads(std::vector<std::string>& args, std::size_t& threads) {
    if (args.size() < 2 || args.front() != "--threads") {
        return true;
    }
    if (!parse_count(args[1], threads) || threads == 0) {
        std::cerr << "search: threads must be a positive integer: " << args[1] << "\n";
        return false;
    }
    args.erase(args.begin(), args.begin() + 2);
    return true;
}

std::optional<InvertedIndex> open_index(const std::filesystem::path& source,
                                        std::size_t threads = 1) {
    std::error_code ec;
    if (std::filesystem::is_directory(source, ec)) {
        return build_index_parallel(source, threads);
    }

    if (std::filesystem::is_regular_file(source, ec)) {
        std::string error;
        std::optional<InvertedIndex> index = read_index_file(source, error);
        if (!index.has_value()) {
            std::cerr << "search: " << error << "\n";
        }
        return index;
    }

    reject_missing_corpus(source);
    return std::nullopt;
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

    const std::optional<InvertedIndex> opened = open_index(args[0]);
    if (!opened.has_value()) {
        return 1;
    }
    const InvertedIndex& index = *opened;

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
    const std::optional<InvertedIndex> opened = open_index(args[0]);
    if (!opened.has_value()) {
        return 1;
    }
    const InvertedIndex& index = *opened;

    const ParseResult parsed = parse_query(query);
    if (!parsed.root) {
        std::cerr << "search: " << parsed.error << "\n";
        return 1;
    }

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

bool read_text_file(const std::filesystem::path& path, std::string& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::cerr << "search: cannot read " << path.string() << "\n";
        return false;
    }
    out.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    return true;
}

bool take_query_options(std::vector<std::string>& args, QueryOptions& options) {
    while (!args.empty() && args.front().starts_with("--")) {
        const std::string flag = args.front();

        if (flag == "--snippet") {
            options.snippets = true;
            args.erase(args.begin());
            continue;
        }
        if (flag == "--tsv") {
            options.tsv = true;
            args.erase(args.begin());
            continue;
        }

        if (args.size() < 2) {
            return false;
        }
        const std::string value = args[1];

        if (flag == "--limit") {
            if (!parse_count(value, options.limit)) {
                std::cerr << "search: limit must be a non-negative integer: " << value << "\n";
                return false;
            }
        } else if (flag == "--max-chars") {
            if (!parse_count(value, options.snippet_chars) || options.snippet_chars == 0) {
                std::cerr << "search: max-chars must be a positive integer: " << value << "\n";
                return false;
            }
        } else if (flag == "--scorer") {
            if (value == "bm25") {
                options.scorer = Scorer::Bm25;
            } else if (value == "tfidf") {
                options.scorer = Scorer::TfIdf;
            } else {
                std::cerr << "search: unknown scorer: " << value << "\n";
                return false;
            }
        } else {
            std::cerr << "search: unknown option: " << flag << "\n";
            return false;
        }

        args.erase(args.begin(), args.begin() + 2);
    }

    return true;
}

std::optional<std::filesystem::path> corpus_for_snippets(const std::string& source) {
    std::error_code ec;
    if (std::filesystem::is_directory(source, ec)) {
        return std::filesystem::path(source);
    }
    return std::nullopt;
}

int cmd_query(const std::vector<std::string>& args) {
    std::vector<std::string> rest = args;
    QueryOptions options;
    if (!take_query_options(rest, options)) {
        return 1;
    }
    if (rest.size() != 2) {
        return usage();
    }

    const std::optional<InvertedIndex> opened = open_index(rest[0]);
    if (!opened.has_value()) {
        return 1;
    }

    std::string error;
    const std::optional<std::vector<QueryResult>> results =
        run_query(*opened, corpus_for_snippets(rest[0]), rest[1], options, error);
    if (!results.has_value()) {
        std::cerr << "search: " << error << "\n";
        return 1;
    }

    print_results(std::cout, *results, options);
    return 0;
}

int repl_meta_command(const std::string& line, QueryOptions& options, bool& quit) {
    std::istringstream words(line);
    std::string name;
    std::string value;
    words >> name >> value;

    if (name == ":quit" || name == ":q") {
        quit = true;
        return 0;
    }
    if (name == ":help") {
        std::cout << "queries use AND, OR, NOT, parentheses and \"phrases\"\n"
                     ":limit <n>      how many results to show\n"
                     ":scorer <name>  bm25 or tfidf\n"
                     ":snippet <on|off>\n"
                     ":settings       show the current settings\n"
                     ":quit\n";
        return 0;
    }
    if (name == ":settings") {
        std::cout << "limit " << options.limit << "\n"
                  << "scorer " << (options.scorer == Scorer::Bm25 ? "bm25" : "tfidf") << "\n"
                  << "snippet " << (options.snippets ? "on" : "off") << "\n";
        return 0;
    }
    if (name == ":limit") {
        if (!parse_count(value, options.limit)) {
            std::cout << "limit must be a non-negative integer\n";
        }
        return 0;
    }
    if (name == ":scorer") {
        if (value == "bm25") {
            options.scorer = Scorer::Bm25;
        } else if (value == "tfidf") {
            options.scorer = Scorer::TfIdf;
        } else {
            std::cout << "scorer must be bm25 or tfidf\n";
        }
        return 0;
    }
    if (name == ":snippet") {
        if (value == "on") {
            options.snippets = true;
        } else if (value == "off") {
            options.snippets = false;
        } else {
            std::cout << "snippet must be on or off\n";
        }
        return 0;
    }

    std::cout << "unknown command: " << name << "\n";
    return 0;
}

int cmd_repl(const std::vector<std::string>& args) {
    std::vector<std::string> rest = args;
    QueryOptions options;
    if (!take_query_options(rest, options)) {
        return 1;
    }
    if (rest.size() != 1) {
        return usage();
    }

    const std::optional<InvertedIndex> opened = open_index(rest[0]);
    if (!opened.has_value()) {
        return 1;
    }

    const std::optional<std::filesystem::path> corpus_dir = corpus_for_snippets(rest[0]);
    const bool interactive = isatty(STDIN_FILENO) != 0;

    std::string line;
    while (true) {
        if (interactive) {
            std::cout << "> " << std::flush;
        }
        if (!std::getline(std::cin, line)) {
            break;
        }

        std::size_t begin = line.find_first_not_of(" \t");
        if (begin == std::string::npos) {
            continue;
        }
        const std::size_t end = line.find_last_not_of(" \t");
        line = line.substr(begin, end - begin + 1);

        if (line.front() == ':') {
            bool quit = false;
            repl_meta_command(line, options, quit);
            if (quit) {
                break;
            }
            continue;
        }

        std::string error;
        const std::optional<std::vector<QueryResult>> results =
            run_query(*opened, corpus_dir, line, options, error);
        if (!results.has_value()) {
            std::cout << "error: " << error << "\n";
            continue;
        }

        print_results(std::cout, *results, options);
    }

    return 0;
}

int cmd_index_update(const std::vector<std::string>& args) {
    std::vector<std::string> rest = args;
    IndexEncoding encoding = IndexEncoding::VarByte;
    if (rest.size() >= 2 && rest.front() == "--encoding") {
        const std::optional<IndexEncoding> parsed = parse_encoding(rest[1]);
        if (!parsed.has_value()) {
            std::cerr << "search: unknown encoding: " << rest[1] << "\n";
            return 1;
        }
        encoding = *parsed;
        rest.erase(rest.begin(), rest.begin() + 2);
    }
    if (rest.size() != 2) {
        return usage();
    }

    const std::filesystem::path corpus_dir = rest[0];
    std::error_code ec;
    if (!std::filesystem::is_directory(corpus_dir, ec)) {
        return reject_missing_corpus(corpus_dir);
    }

    InvertedIndex previous;
    if (std::filesystem::is_regular_file(rest[1], ec)) {
        std::string error;
        std::optional<InvertedIndex> loaded = read_index_file(rest[1], error);
        if (!loaded.has_value()) {
            std::cerr << "search: " << error << "\n";
            return 1;
        }
        previous = std::move(*loaded);
    }

    IndexUpdateReport report;
    const InvertedIndex updated = update_index(previous, corpus_dir, report);

    std::string error;
    if (!write_index_file(updated, rest[1], encoding, error)) {
        std::cerr << "search: " << error << "\n";
        return 1;
    }

    std::cout << "added " << report.added << "\n"
              << "updated " << report.updated << "\n"
              << "removed " << report.removed << "\n"
              << "unchanged " << report.unchanged << "\n";
    return 0;
}

int cmd_pagerank(const std::vector<std::string>& args) {
    std::vector<std::string> rest = args;
    std::size_t iterations = 30;
    double damping = 0.85;

    while (rest.size() >= 2 && rest.front().starts_with("--")) {
        const std::string flag = rest.front();
        if (flag == "--iterations") {
            if (!parse_count(rest[1], iterations)) {
                std::cerr << "search: iterations must be a non-negative integer: " << rest[1]
                          << "\n";
                return 1;
            }
        } else if (flag == "--damping") {
            try {
                std::size_t consumed = 0;
                damping = std::stod(rest[1], &consumed);
                if (consumed != rest[1].size() || damping < 0.0 || damping > 1.0) {
                    throw std::invalid_argument("damping");
                }
            } catch (const std::exception&) {
                std::cerr << "search: damping must be between 0 and 1: " << rest[1] << "\n";
                return 1;
            }
        } else {
            std::cerr << "search: unknown option: " << flag << "\n";
            return 1;
        }
        rest.erase(rest.begin(), rest.begin() + 2);
    }

    if (rest.size() != 1) {
        return usage();
    }

    std::string text;
    if (!read_text_file(rest[0], text)) {
        return 1;
    }

    std::vector<std::pair<std::string, std::string>> links;
    std::size_t line_start = 0;
    while (line_start < text.size()) {
        const std::size_t line_end = text.find('\n', line_start);
        const std::string line =
            text.substr(line_start, line_end == std::string::npos ? std::string::npos
                                                                  : line_end - line_start);
        const std::size_t tab = line.find('\t');
        if (tab != std::string::npos) {
            links.emplace_back(line.substr(0, tab), line.substr(tab + 1));
        }
        if (line_end == std::string::npos) {
            break;
        }
        line_start = line_end + 1;
    }

    std::cout << std::fixed << std::setprecision(6);
    for (const RankedPage& page : pagerank(links, iterations, damping)) {
        std::cout << page.page << " " << page.score << "\n";
    }
    return 0;
}

int cmd_edit_distance(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return usage();
    }

    std::cout << edit_distance(args[0], args[1]) << "\n";
    return 0;
}

int cmd_snippet(const std::vector<std::string>& args) {
    std::vector<std::string> rest = args;
    SnippetOptions options;

    if (rest.size() >= 2 && rest.front() == "--max-chars") {
        std::size_t max_chars = 0;
        if (!parse_count(rest[1], max_chars) || max_chars == 0) {
            std::cerr << "search: max-chars must be a positive integer: " << rest[1] << "\n";
            return 1;
        }
        options.max_chars = max_chars;
        rest.erase(rest.begin(), rest.begin() + 2);
    }

    if (rest.size() < 3) {
        return usage();
    }

    const std::filesystem::path corpus_dir = rest[0];
    std::error_code ec;
    if (!std::filesystem::is_directory(corpus_dir, ec)) {
        return reject_missing_corpus(corpus_dir);
    }

    const std::optional<Document> doc = read_document(corpus_dir, rest[1]);
    if (!doc.has_value()) {
        std::cerr << "search: no such document: " << rest[1] << "\n";
        return 1;
    }

    const std::vector<std::string> terms =
        query_terms(std::vector<std::string>(rest.begin() + 2, rest.end()));

    std::cout << make_snippet(doc->title + "\n\n" + doc->body, terms, options) << "\n";
    return 0;
}

int cmd_suggest(const std::vector<std::string>& args) {
    std::vector<std::string> rest = args;
    std::size_t max_distance = 2;
    std::size_t limit = 5;

    while (rest.size() >= 2 && rest.front().starts_with("--")) {
        const std::string flag = rest.front();
        std::size_t value = 0;
        if (!parse_count(rest[1], value)) {
            std::cerr << "search: expected a number after " << flag << ": " << rest[1] << "\n";
            return 1;
        }
        if (flag == "--max-distance") {
            max_distance = value;
        } else if (flag == "--limit") {
            limit = value;
        } else {
            std::cerr << "search: unknown option: " << flag << "\n";
            return 1;
        }
        rest.erase(rest.begin(), rest.begin() + 2);
    }

    if (rest.size() != 2) {
        return usage();
    }

    const std::optional<InvertedIndex> opened = open_index(rest[0]);
    if (!opened.has_value()) {
        return 1;
    }

    const std::vector<Token> analyzed = analyze(rest[1]);
    if (analyzed.size() != 1) {
        std::cerr << "search: not a single term: " << rest[1] << "\n";
        return 1;
    }

    const BkTree tree = build_bk_tree(*opened);
    for (const Suggestion& suggestion : tree.search(analyzed.front().text, max_distance, limit)) {
        std::cout << suggestion.term << " " << suggestion.distance << " "
                  << suggestion.document_frequency << "\n";
    }
    return 0;
}

int cmd_complete(const std::vector<std::string>& args) {
    std::vector<std::string> rest = args;
    std::size_t limit = 10;

    if (rest.size() >= 2 && rest.front() == "--limit") {
        if (!parse_count(rest[1], limit)) {
            std::cerr << "search: limit must be a non-negative integer: " << rest[1] << "\n";
            return 1;
        }
        rest.erase(rest.begin(), rest.begin() + 2);
    }

    if (rest.size() != 2) {
        return usage();
    }

    const std::optional<InvertedIndex> opened = open_index(rest[0]);
    if (!opened.has_value()) {
        return 1;
    }

    const Trie trie = build_trie(*opened);
    for (const Completion& completion : trie.complete(normalize(rest[1]), limit)) {
        std::cout << completion.term << " " << completion.document_frequency << "\n";
    }
    return 0;
}

int cmd_url(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    const std::optional<Url> url = parse_url(args[0]);
    if (!url.has_value()) {
        std::cerr << "search: not an http url: " << args[0] << "\n";
        return 1;
    }

    std::cout << url_to_string(*url) << "\n";
    return 0;
}

int cmd_url_resolve(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return usage();
    }

    const std::optional<Url> base = parse_url(args[0]);
    if (!base.has_value()) {
        std::cerr << "search: not an http url: " << args[0] << "\n";
        return 1;
    }

    const std::optional<Url> resolved = resolve_url(*base, args[1]);
    if (!resolved.has_value()) {
        std::cerr << "search: cannot resolve: " << args[1] << "\n";
        return 1;
    }

    std::cout << url_to_string(*resolved) << "\n";
    return 0;
}

int cmd_robots(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return usage();
    }

    std::string text;
    if (!read_text_file(args[0], text)) {
        return 1;
    }

    const RobotsRules rules = RobotsRules::parse(text, args[1]);
    std::cout << (rules.allows(args[2]) ? "allow" : "disallow") << "\n";
    return 0;
}

int cmd_html_text(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    std::string text;
    if (!read_text_file(args[0], text)) {
        return 1;
    }

    const HtmlPage page = parse_html(text);
    std::cout << "title: " << page.title << "\n" << page.text << "\n";
    return 0;
}

int cmd_html_links(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return usage();
    }

    const std::optional<Url> base = parse_url(args[0]);
    if (!base.has_value()) {
        std::cerr << "search: not an http url: " << args[0] << "\n";
        return 1;
    }

    std::string text;
    if (!read_text_file(args[1], text)) {
        return 1;
    }

    for (const std::string& href : parse_html(text).links) {
        const std::optional<Url> resolved = resolve_url(*base, href);
        if (resolved.has_value()) {
            std::cout << url_to_string(*resolved) << "\n";
        }
    }
    return 0;
}

int cmd_crawl_delay(const std::vector<std::string>& args) {
    if (args.size() != 3) {
        return usage();
    }

    std::string text;
    if (!read_text_file(args[0], text)) {
        return 1;
    }

    std::size_t floor = 0;
    if (!parse_count(args[2], floor)) {
        std::cerr << "search: floor must be a non-negative integer: " << args[2] << "\n";
        return 1;
    }

    const RobotsRules rules = RobotsRules::parse(text, args[1]);
    std::cout << delay_for(rules, std::chrono::milliseconds(static_cast<long long>(floor))).count()
              << "\n";
    return 0;
}

int cmd_crawl(const std::vector<std::string>& args) {
    std::vector<std::string> rest = args;
    CrawlOptions options;
    std::optional<std::filesystem::path> mirror;
    bool delay_given = false;

    while (rest.size() >= 1 && rest.front().starts_with("--")) {
        const std::string flag = rest.front();

        if (flag == "--all-hosts") {
            options.same_host_only = false;
            rest.erase(rest.begin());
            continue;
        }

        if (rest.size() < 2) {
            return usage();
        }
        const std::string value = rest[1];

        if (flag == "--mirror") {
            mirror = value;
        } else if (flag == "--out") {
            options.output_dir = value;
        } else if (flag == "--user-agent") {
            options.user_agent = value;
        } else if (flag == "--max-pages") {
            if (!parse_count(value, options.max_pages)) {
                std::cerr << "search: max-pages must be a non-negative integer: " << value << "\n";
                return 1;
            }
        } else if (flag == "--delay") {
            std::size_t milliseconds = 0;
            if (!parse_count(value, milliseconds)) {
                std::cerr << "search: delay must be a non-negative integer: " << value << "\n";
                return 1;
            }
            options.min_delay = std::chrono::milliseconds(static_cast<long long>(milliseconds));
            delay_given = true;
        } else if (flag == "--max-depth") {
            if (!parse_count(value, options.max_depth)) {
                std::cerr << "search: max-depth must be a non-negative integer: " << value << "\n";
                return 1;
            }
        } else {
            std::cerr << "search: unknown option: " << flag << "\n";
            return 1;
        }

        rest.erase(rest.begin(), rest.begin() + 2);
    }

    if (rest.size() != 1) {
        return usage();
    }

    const std::optional<Url> seed = parse_url(rest[0]);
    if (!seed.has_value()) {
        std::cerr << "search: not an http url: " << rest[0] << "\n";
        return 1;
    }

    std::unique_ptr<Fetcher> fetcher;
    if (mirror.has_value() && !delay_given) {
        options.min_delay = std::chrono::milliseconds(0);
    }
    if (mirror.has_value()) {
        std::error_code ec;
        if (!std::filesystem::is_directory(*mirror, ec)) {
            return reject_missing_corpus(*mirror);
        }
        fetcher = make_file_fetcher(*mirror);
    } else {
        fetcher = make_http_fetcher(options.user_agent, 10);
        if (fetcher == nullptr) {
            std::cerr << "search: this build cannot fetch over http; pass --mirror\n";
            return 1;
        }
    }

    for (const CrawledPage& page : crawl(*seed, *fetcher, options)) {
        std::cout << page.depth << " " << page.status << " " << page.url << "\n";
    }
    return 0;
}

int cmd_pool_sum(const std::vector<std::string>& args) {
    if (args.size() != 2) {
        return usage();
    }

    std::size_t threads = 0;
    std::size_t count = 0;
    if (!parse_count(args[0], threads) || threads == 0) {
        std::cerr << "search: threads must be a positive integer: " << args[0] << "\n";
        return 1;
    }
    if (!parse_count(args[1], count)) {
        std::cerr << "search: count must be a non-negative integer: " << args[1] << "\n";
        return 1;
    }

    ThreadPool pool(threads);
    std::vector<std::future<std::size_t>> pending;
    pending.reserve(count);
    for (std::size_t i = 0; i < count; i++) {
        pending.push_back(pool.submit([i] { return i * i; }));
    }

    std::size_t total = 0;
    for (std::future<std::size_t>& future : pending) {
        total += future.get();
    }

    std::cout << "threads " << pool.size() << "\n"
              << "tasks " << count << "\n"
              << "sum " << total << "\n";
    return 0;
}

int cmd_index_write(const std::vector<std::string>& args) {
    std::vector<std::string> rest = args;
    IndexEncoding encoding = IndexEncoding::VarByte;
    if (rest.size() >= 2 && rest.front() == "--encoding") {
        const std::optional<IndexEncoding> parsed = parse_encoding(rest[1]);
        if (!parsed.has_value()) {
            std::cerr << "search: unknown encoding: " << rest[1] << "\n";
            return 1;
        }
        encoding = *parsed;
        rest.erase(rest.begin(), rest.begin() + 2);
    }
    std::size_t threads = 1;
    if (!take_threads(rest, threads)) {
        return 1;
    }
    if (rest.size() != 2) {
        return usage();
    }

    const std::optional<InvertedIndex> opened = open_index(rest[0], threads);
    if (!opened.has_value()) {
        return 1;
    }

    std::string error;
    if (!write_index_file(*opened, rest[1], encoding, error)) {
        std::cerr << "search: " << error << "\n";
        return 1;
    }
    return 0;
}

int cmd_index_size(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    const std::optional<InvertedIndex> opened = open_index(args[0]);
    if (!opened.has_value()) {
        return 1;
    }

    const IndexEncoding encodings[] = {IndexEncoding::Plain, IndexEncoding::Delta,
                                       IndexEncoding::VarByte};
    const char* names[] = {"plain", "delta", "varbyte"};

    std::size_t plain_total = 0;
    for (std::size_t i = 0; i < 3; i++) {
        const IndexSizeReport report = measure_index(*opened, encodings[i]);
        if (i == 0) {
            plain_total = report.total;
        }

        std::cout << names[i] << " " << report.total
                  << " header " << report.header
                  << " documents " << report.documents
                  << " dictionary " << report.dictionary
                  << " postings " << report.postings
                  << " positions " << report.positions << "\n";
    }

    const IndexSizeReport best = measure_index(*opened, IndexEncoding::VarByte);
    std::cout << "ratio " << std::fixed << std::setprecision(2)
              << (best.total > 0 ? static_cast<double>(plain_total) / static_cast<double>(best.total)
                                 : 0.0)
              << "\n";
    return 0;
}

int cmd_lengths(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    const std::optional<InvertedIndex> opened = open_index(args[0]);
    if (!opened.has_value()) {
        return 1;
    }
    const InvertedIndex& index = *opened;

    const std::vector<std::string>& ids = index.document_ids();
    for (std::size_t doc_id = 0; doc_id < ids.size(); doc_id++) {
        std::cout << ids[doc_id] << " " << index.document_length(doc_id) << "\n";
    }

    std::cout << "average " << std::fixed << std::setprecision(2)
              << index.average_document_length() << "\n";
    return 0;
}

void print_ranking(const InvertedIndex& index, const std::vector<ScoredDocument>& ranking) {
    const std::vector<std::string>& ids = index.document_ids();
    std::cout << std::fixed << std::setprecision(4);
    for (const ScoredDocument& scored : ranking) {
        std::cout << ids[scored.doc_id] << " " << scored.score << "\n";
    }
}

int run_ranking(const std::vector<std::string>& args, Scorer scorer) {
    std::vector<std::string> rest = args;
    std::size_t threads = 1;
    if (!take_threads(rest, threads)) {
        return 1;
    }
    if (rest.size() < 2) {
        return usage();
    }

    const std::optional<InvertedIndex> opened = open_index(rest[0], threads);
    if (!opened.has_value()) {
        return 1;
    }
    const InvertedIndex& index = *opened;

    const std::vector<std::string> terms =
        query_terms(std::vector<std::string>(rest.begin() + 1, rest.end()));

    print_ranking(index, score(terms, index, scorer, threads));
    return 0;
}

int cmd_tfidf(const std::vector<std::string>& args) {
    return run_ranking(args, Scorer::TfIdf);
}

int cmd_bm25(const std::vector<std::string>& args) {
    return run_ranking(args, Scorer::Bm25);
}

int cmd_top(const std::vector<std::string>& args) {
    std::vector<std::string> rest = args;
    bool use_bm25 = false;
    if (!rest.empty() && rest.front() == "--bm25") {
        use_bm25 = true;
        rest.erase(rest.begin());
    }
    std::size_t threads = 1;
    if (!take_threads(rest, threads)) {
        return 1;
    }
    if (rest.size() < 3) {
        return usage();
    }

    const std::optional<InvertedIndex> opened = open_index(rest[0], threads);
    if (!opened.has_value()) {
        return 1;
    }
    const InvertedIndex& index = *opened;

    std::size_t k = 0;
    if (!parse_count(rest[1], k)) {
        std::cerr << "search: k must be a non-negative integer: " << rest[1] << "\n";
        return 1;
    }

    const std::vector<std::string> terms =
        query_terms(std::vector<std::string>(rest.begin() + 2, rest.end()));

    const std::vector<ScoredDocument> ranking =
        score(terms, index, use_bm25 ? Scorer::Bm25 : Scorer::TfIdf, threads);

    print_ranking(index, top_k(ranking, k));
    return 0;
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

    const std::optional<InvertedIndex> opened = open_index(args[0]);
    if (!opened.has_value()) {
        return 1;
    }
    const InvertedIndex& index = *opened;

    const std::vector<Token> query = analyze(args[1]);
    if (query.size() > 1) {
        std::cerr << "search: not a single term: " << args[1] << "\n";
        return 1;
    }
    if (query.empty()) {
        return 0;
    }

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

    const std::optional<InvertedIndex> opened = open_index(args[0]);
    if (!opened.has_value()) {
        return 1;
    }
    const InvertedIndex& index = *opened;

    std::cout << "documents: " << index.document_count() << "\n"
              << "terms: " << index.term_count() << "\n"
              << "postings: " << index.posting_count() << "\n";
    return 0;
}

int cmd_index_terms(const std::vector<std::string>& args) {
    if (args.size() != 1) {
        return usage();
    }

    const std::optional<InvertedIndex> opened = open_index(args[0]);
    if (!opened.has_value()) {
        return 1;
    }
    const InvertedIndex& index = *opened;

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
    if (command == "lengths") {
        return cmd_lengths(rest);
    }
    if (command == "query") {
        return cmd_query(rest);
    }
    if (command == "repl") {
        return cmd_repl(rest);
    }
    if (command == "index-update") {
        return cmd_index_update(rest);
    }
    if (command == "pagerank") {
        return cmd_pagerank(rest);
    }
    if (command == "edit-distance") {
        return cmd_edit_distance(rest);
    }
    if (command == "snippet") {
        return cmd_snippet(rest);
    }
    if (command == "suggest") {
        return cmd_suggest(rest);
    }
    if (command == "complete") {
        return cmd_complete(rest);
    }
    if (command == "url") {
        return cmd_url(rest);
    }
    if (command == "url-resolve") {
        return cmd_url_resolve(rest);
    }
    if (command == "robots") {
        return cmd_robots(rest);
    }
    if (command == "html-text") {
        return cmd_html_text(rest);
    }
    if (command == "html-links") {
        return cmd_html_links(rest);
    }
    if (command == "crawl-delay") {
        return cmd_crawl_delay(rest);
    }
    if (command == "crawl") {
        return cmd_crawl(rest);
    }
    if (command == "pool-sum") {
        return cmd_pool_sum(rest);
    }
    if (command == "index-write") {
        return cmd_index_write(rest);
    }
    if (command == "index-size") {
        return cmd_index_size(rest);
    }
    if (command == "tfidf") {
        return cmd_tfidf(rest);
    }
    if (command == "bm25") {
        return cmd_bm25(rest);
    }
    if (command == "top") {
        return cmd_top(rest);
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
