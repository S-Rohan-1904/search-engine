#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>

// A whole file, mapped read-only into the address space.
//
// The corpus is read once per index build and then only indexed into, which is
// what a mapping is for: no 400 MB copy into the heap, no wait for the read to
// finish before the first document can be analyzed, and pages the build never
// touches are never faulted in. The pages are file-backed and clean, so they
// also do not count the way an equally large heap buffer would.
class MappedFile {
public:
    MappedFile() = default;
    ~MappedFile();

    MappedFile(MappedFile&& other) noexcept;
    MappedFile& operator=(MappedFile&& other) noexcept;
    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    // Maps the whole file, or returns nullopt if it cannot be opened. An empty
    // file maps to an empty region rather than failing, since what it means for
    // a caller is "no contents", not "no file".
    static std::optional<MappedFile> open(const std::filesystem::path& path);

    const unsigned char* data() const { return data_; }
    std::size_t size() const { return size_; }

private:
    const unsigned char* data_ = nullptr;
    std::size_t size_ = 0;
};
