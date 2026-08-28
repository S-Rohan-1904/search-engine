#include "snippet.hpp"

#include <algorithm>
#include <cctype>
#include <unordered_set>

#include "analyzer.hpp"
#include "tokenizer.hpp"

namespace {

struct Word {
    std::size_t begin;
    std::size_t end;
    std::size_t term_index;  // index into the query terms, or npos
    std::size_t highlight_begin = 0;
    std::size_t highlight_end = 0;
};

constexpr std::size_t kNoMatch = static_cast<std::size_t>(-1);

bool is_word_byte(char c) {
    const unsigned char byte = static_cast<unsigned char>(c);
    return byte >= 0x80 || std::isalnum(byte) != 0;
}

std::vector<Word> mark_words(std::string_view text, const std::vector<std::string>& terms) {
    std::unordered_set<std::string> wanted(terms.begin(), terms.end());
    std::vector<Word> words;

    for (const Token& token : tokenize(text)) {
        std::size_t begin = token.offset;
        std::size_t end = token.offset + token.text.size();
        while (begin < end && !is_word_byte(text[begin])) {
            begin++;
        }
        while (end > begin && !is_word_byte(text[end - 1])) {
            end--;
        }

        Word word{token.offset, token.offset + token.text.size(), kNoMatch};
        word.highlight_begin = begin;
        word.highlight_end = end;

        const std::vector<Token> analyzed = analyze(token.text);
        if (analyzed.size() == 1 && wanted.count(analyzed.front().text) != 0) {
            for (std::size_t i = 0; i < terms.size(); i++) {
                if (terms[i] == analyzed.front().text) {
                    word.term_index = i;
                    break;
                }
            }
        }

        words.push_back(word);
    }

    return words;
}

std::string render(std::string_view text, const std::vector<Word>& words, std::size_t first,
                   std::size_t last, const SnippetOptions& options) {
    std::string out;

    if (words[first].begin > 0) {
        out += "...";
    }

    std::size_t cursor = words[first].begin;
    for (std::size_t i = first; i <= last; i++) {
        out.append(text.substr(cursor, words[i].begin - cursor));
        if (words[i].term_index != kNoMatch && words[i].highlight_end > words[i].highlight_begin) {
            out.append(text.substr(words[i].begin, words[i].highlight_begin - words[i].begin));
            out += options.open;
            out.append(text.substr(words[i].highlight_begin,
                                   words[i].highlight_end - words[i].highlight_begin));
            out += options.close;
            out.append(text.substr(words[i].highlight_end, words[i].end - words[i].highlight_end));
        } else {
            out.append(text.substr(words[i].begin, words[i].end - words[i].begin));
        }
        cursor = words[i].end;
    }

    if (words[last].end < text.size()) {
        out += "...";
    }

    return out;
}

}

std::string make_snippet(std::string_view text, const std::vector<std::string>& terms,
                         const SnippetOptions& options) {
    const std::vector<Word> words = mark_words(text, terms);
    if (words.empty()) {
        return {};
    }

    std::size_t best_first = 0;
    std::size_t best_last = 0;
    std::size_t best_distinct = 0;
    std::size_t best_total = 0;

    for (std::size_t first = 0; first < words.size(); first++) {
        std::vector<bool> seen(terms.size(), false);
        std::size_t distinct = 0;
        std::size_t total = 0;
        std::size_t last = first;

        for (std::size_t i = first; i < words.size(); i++) {
            if (words[i].end - words[first].begin > options.max_chars) {
                break;
            }
            if (words[i].term_index != kNoMatch) {
                total++;
                if (!seen[words[i].term_index]) {
                    seen[words[i].term_index] = true;
                    distinct++;
                }
            }
            last = i;
        }

        if (distinct > best_distinct || (distinct == best_distinct && total > best_total)) {
            best_distinct = distinct;
            best_total = total;
            best_first = first;
            best_last = last;
        }
    }

    if (best_distinct == 0) {
        best_first = 0;
        best_last = 0;
        for (std::size_t i = 0; i < words.size(); i++) {
            if (words[i].end - words[0].begin > options.max_chars) {
                break;
            }
            best_last = i;
        }
    }

    return render(text, words, best_first, best_last, options);
}
