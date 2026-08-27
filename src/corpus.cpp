#include "corpus.hpp"

#include <algorithm>

std::vector<std::string> list_document_ids(const std::filesystem::path& corpus_dir) {
    std::vector<std::string> document_ids;

    for(const auto & entry : std::filesystem::directory_iterator(corpus_dir)) {
        if(entry.is_regular_file() && entry.path().extension() == ".txt") {
            document_ids.push_back(entry.path().stem().string());
        }
    }

    std::sort(document_ids.begin(), document_ids.end());

    return document_ids;
}
