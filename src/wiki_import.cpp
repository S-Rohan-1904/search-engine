#include "wiki_import.hpp"

#include <fstream>
#include <iomanip>
#include <istream>
#include <optional>
#include <sstream>
#include <string_view>

#include "corpus_file.hpp"
#include "jsonl.hpp"

namespace {

std::string document_id(std::size_t ordinal) {
    std::ostringstream name;
    name << "doc_" << std::setw(8) << std::setfill('0') << ordinal;
    return name.str();
}

}

bool import_cirrussearch(std::istream& in, const std::filesystem::path& output, WikiField field,
                         std::size_t limit, WikiImportReport& report, std::string& error) {
    report = WikiImportReport{};
    error.clear();

    std::optional<CorpusWriter> writer = CorpusWriter::create(output, error);
    if (!writer.has_value()) {
        return false;
    }

    const std::string_view body_key = field == WikiField::OpeningText ? "opening_text" : "text";

    std::string line;
    while (std::getline(in, line)) {
        report.lines_seen++;

        const std::optional<std::string> title = field_of(line, "title");
        if (!title.has_value()) {
            continue;
        }

        const std::optional<std::string> body = field_of(line, body_key);
        const std::string text = body.has_value() ? collapse(*body) : std::string();
        if (text.empty()) {
            report.skipped_empty++;
            continue;
        }

        const std::string document =
            "title: " + collapse(*title) + "\n\n" + text + "\n";
        if (!writer->add(document_id(report.documents_written + 1), document)) {
            error = "cannot write " + output.string();
            return false;
        }
        report.documents_written++;

        if (limit != 0 && report.documents_written >= limit) {
            break;
        }
    }

    return writer->finish(error);
}
