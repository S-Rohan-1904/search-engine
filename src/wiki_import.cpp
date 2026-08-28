#include "wiki_import.hpp"

#include <fstream>
#include <iomanip>
#include <istream>
#include <optional>
#include <sstream>
#include <string_view>

#include "corpus_file.hpp"

namespace {

void append_utf8(std::string& out, unsigned long code) {
    if (code < 0x80) {
        out.push_back(static_cast<char>(code));
    } else if (code < 0x800) {
        out.push_back(static_cast<char>(0xC0 | (code >> 6)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
    } else if (code < 0x10000) {
        out.push_back(static_cast<char>(0xE0 | (code >> 12)));
        out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
    } else {
        out.push_back(static_cast<char>(0xF0 | (code >> 18)));
        out.push_back(static_cast<char>(0x80 | ((code >> 12) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3F)));
        out.push_back(static_cast<char>(0x80 | (code & 0x3F)));
    }
}

bool read_hex4(std::string_view text, std::size_t at, unsigned long& out) {
    if (text.size() - at < 4) {
        return false;
    }

    out = 0;
    for (std::size_t i = 0; i < 4; i++) {
        const char c = text[at + i];
        int digit = 0;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            return false;
        }
        out = out * 16 + static_cast<unsigned long>(digit);
    }
    return true;
}
bool is_json_space(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

std::size_t skip_spaces(std::string_view line, std::size_t at) {
    while (at < line.size() && is_json_space(line[at])) {
        at++;
    }
    return at;
}

std::size_t find_key(std::string_view line, std::string_view key) {
    std::string needle = "\"";
    needle += key;
    needle += "\"";

    std::size_t at = 0;
    while (true) {
        const std::size_t found = line.find(needle, at);
        if (found == std::string_view::npos) {
            return std::string_view::npos;
        }
        at = found + 1;

        std::size_t before = found;
        while (before > 0 && is_json_space(line[before - 1])) {
            before--;
        }
        if (before == 0 || (line[before - 1] != '{' && line[before - 1] != ',')) {
            continue;
        }

        std::size_t after = skip_spaces(line, found + needle.size());
        if (after >= line.size() || line[after] != ':') {
            continue;
        }

        after = skip_spaces(line, after + 1);
        if (after >= line.size() || line[after] != '"') {
            continue;
        }

        return after + 1;
    }
}

std::string read_string(std::string_view line, std::size_t at) {
    std::string out;

    while (at < line.size()) {
        const char c = line[at];
        if (c == '"') {
            break;
        }

        if (c != '\\') {
            out.push_back(c);
            at++;
            continue;
        }

        at++;
        if (at >= line.size()) {
            break;
        }

        switch (line[at]) {
        case 'n': out.push_back('\n'); at++; break;
        case 't': out.push_back('\t'); at++; break;
        case 'r': out.push_back('\r'); at++; break;
        case 'b': out.push_back('\b'); at++; break;
        case 'f': out.push_back('\f'); at++; break;
        case '"': out.push_back('"'); at++; break;
        case '\\': out.push_back('\\'); at++; break;
        case '/': out.push_back('/'); at++; break;
        case 'u': {
            unsigned long code = 0;
            if (!read_hex4(line, at + 1, code)) {
                out.push_back('\\');
                at++;
                break;
            }
            at += 5;

            if (code >= 0xD800 && code <= 0xDBFF && at + 1 < line.size() && line[at] == '\\' &&
                line[at + 1] == 'u') {
                unsigned long low = 0;
                if (read_hex4(line, at + 2, low) && low >= 0xDC00 && low <= 0xDFFF) {
                    code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                    at += 6;
                }
            }

            append_utf8(out, code);
            break;
        }
        default:
            out.push_back(line[at]);
            at++;
            break;
        }
    }

    return out;
}

std::optional<std::string> field_of(std::string_view line, std::string_view key) {
    const std::size_t at = find_key(line, key);
    if (at == std::string_view::npos) {
        return std::nullopt;
    }
    return read_string(line, at);
}

std::string collapse(std::string_view text) {
    std::string out;
    out.reserve(text.size());

    bool space = false;
    for (const char c : text) {
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\b') {
            space = !out.empty();
            continue;
        }
        if (space) {
            out.push_back(' ');
            space = false;
        }
        out.push_back(c);
    }
    return out;
}

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
