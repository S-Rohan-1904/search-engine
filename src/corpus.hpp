#pragma once

#include <filesystem>
#include <string>
#include <vector>

// Lists the ids of every .txt file directly inside corpus_dir, sorted
// ascending. Subdirectories and other extensions are ignored. An id is the
// filename without the extension: corpus/doc_007.txt -> "doc_007".
std::vector<std::string> list_document_ids(const std::filesystem::path& corpus_dir);
