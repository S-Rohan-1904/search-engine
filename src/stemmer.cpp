#include "stemmer.hpp"

#include <string>
#include <string_view>
#include <utility>

namespace porter {

bool is_consonant(std::string_view word, std::size_t index) {
    switch (word[index])
    {
    case 'a':
    case 'e':
    case 'i':
    case 'o':
    case 'u':
        return false;
    case 'y':
        return index == 0 || !is_consonant(word, index - 1);
    default:
        return true;
    }
}

std::size_t measure(std::string_view word) {
    std::string runs;

    for (std::size_t i = 0; i < word.size(); i++) {
        const char kind = is_consonant(word, i) ? 'C' : 'V';
        if (runs.empty() || runs.back() != kind) {
            runs.push_back(kind);
        }
    }

    if (!runs.empty() && runs.front() == 'C') {
        runs.erase(runs.begin());
    }
    
    if (!runs.empty() && runs.back() == 'V') {
        runs.pop_back();
    }

    return runs.size() / 2;
}

bool contains_vowel(std::string_view word) {
    for (std::size_t i = 0; i < word.size(); i++) {
        if (!is_consonant(word, i)) return true;
    }

    return false;
}

bool ends_with_double_consonant(std::string_view word) {
    const std::size_t word_len = word.size();

    if (word_len < 2) return false;

    return word[word_len - 1] == word[word_len - 2] && is_consonant(word, word_len - 1);
}

bool ends_cvc(std::string_view word) {
    const std::size_t word_len = word.size();

    if (word_len < 3) return false;

    if (!is_consonant(word, word_len - 1)) return false;
    if (is_consonant(word, word_len - 2)) return false;
    if (!is_consonant(word, word_len - 3)) return false;

    const char last = word[word_len - 1];

    return last != 'w' && last != 'x' && last != 'y';
}

std::string step_1a_plurals(std::string_view word) {
    if (word.ends_with("sses")) {
        word.remove_suffix(2);
    } else if (word.ends_with("ies")) {
        word.remove_suffix(2);
    } else if (!word.ends_with("ss") && word.ends_with("s")) {
        word.remove_suffix(1);
    }

    return std::string(word);
}

std::string step_1b_verb_endings(std::string_view word) {
    if (word.ends_with("eed")) {
        std::string_view stem = word.substr(0, word.size() - 3);
        if (measure(stem) > 0) {
            word.remove_suffix(1);
        }
        return std::string(word);
    }

    std::string result;

    if (word.ends_with("ed") && contains_vowel(word.substr(0, word.size() - 2))) {
        result = std::string(word.substr(0, word.size() - 2));
    } else if (word.ends_with("ing") && contains_vowel(word.substr(0, word.size() - 3))) {
        result = std::string(word.substr(0, word.size() - 3));
    } else {
        return std::string(word);
    }

    if (result.ends_with("at") || result.ends_with("bl") || result.ends_with("iz")) {
        result.push_back('e');
    } else if (ends_with_double_consonant(result)) {
        const char last = result.back();
        if (last != 'l' && last != 's' && last != 'z') {
            result.pop_back();
        }
    } else if (measure(result) == 1 && ends_cvc(result)) {
        result.push_back('e');
    }

    return result;
}

namespace {
bool try_rule(std::string& word, std::string_view suffix,
              std::string_view replacement, std::size_t min_measure) {
    if (!std::string_view(word).ends_with(suffix)) {
        return false;
    }

    const std::size_t stem_len = word.size() - suffix.size();
    if (measure(std::string_view(word).substr(0, stem_len)) > min_measure) {
        word.resize(stem_len);
        word.append(replacement);
    }

    return true;
}

}

std::string step_1c_y_to_i(std::string_view word) {
    if (!word.ends_with("y") || !contains_vowel(word.substr(0, word.size() - 1))) {
        return std::string(word);
    }

    std::string result(word);
    result.back() = 'i';
    return result;
}

std::string step_2_double_suffixes(std::string_view word) {
    static constexpr std::pair<std::string_view, std::string_view> kRules[] = {
        {"ational", "ate"}, {"tional", "tion"}, {"enci", "ence"},
        {"anci", "ance"},   {"izer", "ize"},    {"abli", "able"},
        {"alli", "al"},     {"entli", "ent"},   {"eli", "e"},
        {"ousli", "ous"},   {"ization", "ize"}, {"ation", "ate"},
        {"ator", "ate"},    {"alism", "al"},    {"iveness", "ive"},
        {"fulness", "ful"}, {"ousness", "ous"}, {"aliti", "al"},
        {"iviti", "ive"},   {"biliti", "ble"},
    };

    std::string result(word);
    for (const auto& [suffix, replacement] : kRules) {
        if (try_rule(result, suffix, replacement, 0)) break;
    }

    return result;
}

std::string step_3_derived_suffixes(std::string_view word) {
    static constexpr std::pair<std::string_view, std::string_view> kRules[] = {
        {"icate", "ic"}, {"ative", ""}, {"alize", "al"}, {"iciti", "ic"},
        {"ical", "ic"},  {"ful", ""},   {"ness", ""},
    };

    std::string result(word);
    for (const auto& [suffix, replacement] : kRules) {
        if (try_rule(result, suffix, replacement, 0)) break;
    }

    return result;
}

std::string step_4_residual_suffixes(std::string_view word) {
    static constexpr std::string_view kSuffixes[] = {
        "al", "ance", "ence", "er", "ic", "able", "ible", "ant", "ement",
        "ment", "ent", "ou", "ism", "ate", "iti", "ous", "ive", "ize",
    };

    std::string result(word);

    if (result.ends_with("ion")) {
        const std::string_view stem = std::string_view(result).substr(0, result.size() - 3);
        if (!stem.empty() && (stem.back() == 's' || stem.back() == 't')) {
            try_rule(result, "ion", "", 1);
        }
        return result;
    }

    for (std::string_view suffix : kSuffixes) {
        if (try_rule(result, suffix, "", 1)) break;
    }

    return result;
}

std::string step_5a_final_e(std::string_view word) {
    if (!word.ends_with("e")) {
        return std::string(word);
    }

    const std::string_view stem = word.substr(0, word.size() - 1);
    const std::size_t m = measure(stem);

    if (m > 1 || (m == 1 && !ends_cvc(stem))) {
        return std::string(stem);
    }

    return std::string(word);
}

std::string step_5b_double_l(std::string_view word) {
    if (word.ends_with("ll") && measure(word) > 1) {
        return std::string(word.substr(0, word.size() - 1));
    }

    return std::string(word);
}

std::string stem(std::string_view word) {
    if (word.size() <= 2) {
        return std::string(word);
    }

    std::string result = step_1a_plurals(word);
    result = step_1b_verb_endings(result);
    result = step_1c_y_to_i(result);
    result = step_2_double_suffixes(result);
    result = step_3_derived_suffixes(result);
    result = step_4_residual_suffixes(result);
    result = step_5a_final_e(result);
    result = step_5b_double_l(result);
    return result;
}

}
