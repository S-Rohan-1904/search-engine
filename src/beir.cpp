#include "beir.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

#include "corpus_file.hpp"
#include "jsonl.hpp"

namespace {

// A BEIR record: its id, and the text to index, which is the title and the
// body together the way the benchmark's own baselines index them.
struct Record {
    std::string id;
    std::string text;
};

bool read_records(const std::filesystem::path& path, std::vector<Record>& out, std::string& error,
                  std::size_t& lines_seen, std::size_t& skipped_empty) {
    std::ifstream in(path);
    if (!in) {
        error = "cannot read " + path.string();
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        lines_seen++;

        const std::optional<std::string> id = field_of(line, "_id");
        if (!id.has_value() || id->empty()) {
            skipped_empty++;
            continue;
        }

        const std::optional<std::string> title = field_of(line, "title");
        const std::optional<std::string> text = field_of(line, "text");

        std::string body = collapse(text.value_or(""));
        const std::string heading = collapse(title.value_or(""));
        if (heading.empty() && body.empty()) {
            skipped_empty++;
            continue;
        }

        out.push_back(Record{*id, heading.empty() ? body : heading + "\n\n" + body});
    }

    return true;
}

// The stored form of a document: the title line the corpus reader expects,
// then the body.
std::string as_document(const Record& record) {
    const std::size_t split = record.text.find("\n\n");
    if (split == std::string::npos) {
        return "title: \n\n" + record.text;
    }
    return "title: " + record.text.substr(0, split) + "\n\n" + record.text.substr(split + 2);
}

// query id -> (document id -> graded relevance)
using Qrels = std::unordered_map<std::string, std::unordered_map<std::string, int>>;

bool read_qrels(const std::filesystem::path& path, Qrels& out, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "cannot read " + path.string();
        return false;
    }

    std::string line;
    bool first = true;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        if (first) {
            first = false;
            if (line.starts_with("query-id")) {
                continue;
            }
        }

        std::istringstream fields(line);
        std::string query_id;
        std::string document_id;
        std::string score;
        if (!std::getline(fields, query_id, '\t') ||
            !std::getline(fields, document_id, '\t') || !std::getline(fields, score, '\t')) {
            continue;
        }

        int graded = 0;
        try {
            graded = std::stoi(score);
        } catch (const std::exception&) {
            continue;
        }
        if (graded > 0) {
            out[query_id][document_id] = graded;
        }
    }

    return true;
}

double gain(int graded) {
    return std::pow(2.0, static_cast<double>(graded)) - 1.0;
}

double discount(std::size_t rank) {
    return 1.0 / std::log2(static_cast<double>(rank) + 2.0);
}

// nDCG at `cutoff`: the ranking's discounted gain over the best possible
// discounted gain for the same judgements.
double ndcg(const std::vector<QueryResult>& results,
            const std::unordered_map<std::string, int>& judged, std::size_t cutoff) {
    double actual = 0.0;
    for (std::size_t i = 0; i < results.size() && i < cutoff; i++) {
        const auto found = judged.find(results[i].document_id);
        if (found != judged.end()) {
            actual += gain(found->second) * discount(i);
        }
    }

    std::vector<int> ideal;
    ideal.reserve(judged.size());
    for (const auto& [document_id, graded] : judged) {
        ideal.push_back(graded);
    }
    std::sort(ideal.begin(), ideal.end(), std::greater<int>());

    double best = 0.0;
    for (std::size_t i = 0; i < ideal.size() && i < cutoff; i++) {
        best += gain(ideal[i]) * discount(i);
    }

    return best > 0.0 ? actual / best : 0.0;
}

}

bool import_beir(const std::filesystem::path& corpus_jsonl, const std::filesystem::path& output,
                 BeirImportReport& report, std::string& error) {
    report = BeirImportReport{};
    error.clear();

    std::vector<Record> records;
    if (!read_records(corpus_jsonl, records, error, report.lines_seen, report.skipped_empty)) {
        return false;
    }

    std::sort(records.begin(), records.end(),
              [](const Record& a, const Record& b) { return a.id < b.id; });

    std::optional<CorpusWriter> writer = CorpusWriter::create(output, error);
    if (!writer.has_value()) {
        return false;
    }

    for (const Record& record : records) {
        if (!writer->add(record.id, as_document(record))) {
            error = "cannot write " + output.string();
            return false;
        }
        report.documents_written++;
    }

    return writer->finish(error);
}

bool evaluate_relevance(const InvertedIndex& index, const std::filesystem::path& queries_jsonl,
                        const std::filesystem::path& qrels_tsv, const QueryOptions& options,
                        RelevanceReport& report, std::string& error) {
    report = RelevanceReport{};
    error.clear();

    Qrels qrels;
    if (!read_qrels(qrels_tsv, qrels, error)) {
        return false;
    }

    std::ifstream in(queries_jsonl);
    if (!in) {
        error = "cannot read " + queries_jsonl.string();
        return false;
    }

    double precision_total = 0.0;
    double recall_total = 0.0;
    double ndcg_total = 0.0;

    std::string line;
    while (std::getline(in, line)) {
        const std::optional<std::string> id = field_of(line, "_id");
        const std::optional<std::string> text = field_of(line, "text");
        if (!id.has_value() || !text.has_value()) {
            continue;
        }

        const auto judged = qrels.find(*id);
        if (judged == qrels.end()) {
            report.unjudged++;
            continue;
        }

        // A benchmark query is a sentence, not a boolean expression, and its
        // punctuation would be parsed as query syntax. Every word is taken as
        // an OR term, which is what a bag-of-words BM25 baseline does.
        std::string terms;
        for (const char c : *text) {
            terms.push_back((std::isalnum(static_cast<unsigned char>(c)) != 0) ? c : ' ');
        }

        std::istringstream words(terms);
        std::string word;
        std::string expression;
        while (words >> word) {
            if (!expression.empty()) {
                expression += " OR ";
            }
            expression += word;
        }
        if (expression.empty()) {
            report.unjudged++;
            continue;
        }

        QueryOptions run = options;
        run.limit = 100;
        run.snippets = false;

        std::string query_error;
        const std::optional<std::vector<QueryResult>> results =
            run_query(index, nullptr, expression, run, query_error);
        if (!results.has_value()) {
            continue;
        }

        std::size_t hits_at_10 = 0;
        for (std::size_t i = 0; i < results->size() && i < 10; i++) {
            if (judged->second.count((*results)[i].document_id) != 0) {
                hits_at_10++;
            }
        }

        std::size_t hits_at_100 = 0;
        for (const QueryResult& result : *results) {
            if (judged->second.count(result.document_id) != 0) {
                hits_at_100++;
            }
        }

        precision_total += static_cast<double>(hits_at_10) / 10.0;
        recall_total += static_cast<double>(hits_at_100) /
                        static_cast<double>(judged->second.size());
        ndcg_total += ndcg(*results, judged->second, 10);
        report.queries++;
    }

    if (report.queries > 0) {
        const double count = static_cast<double>(report.queries);
        report.precision_at_10 = precision_total / count;
        report.recall_at_100 = recall_total / count;
        report.ndcg_at_10 = ndcg_total / count;
    }

    return true;
}
