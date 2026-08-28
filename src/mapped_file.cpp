#include "mapped_file.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

MappedFile::~MappedFile() {
    if (data_ != nullptr && size_ > 0) {
        munmap(const_cast<unsigned char*>(data_), size_);
    }
}

MappedFile::MappedFile(MappedFile&& other) noexcept : data_(other.data_), size_(other.size_) {
    other.data_ = nullptr;
    other.size_ = 0;
}

MappedFile& MappedFile::operator=(MappedFile&& other) noexcept {
    if (this != &other) {
        if (data_ != nullptr && size_ > 0) {
            munmap(const_cast<unsigned char*>(data_), size_);
        }
        data_ = other.data_;
        size_ = other.size_;
        other.data_ = nullptr;
        other.size_ = 0;
    }
    return *this;
}

std::optional<MappedFile> MappedFile::open(const std::filesystem::path& path) {
    const int fd = ::open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return std::nullopt;
    }

    struct stat info {};
    if (fstat(fd, &info) != 0 || !S_ISREG(info.st_mode)) {
        ::close(fd);
        return std::nullopt;
    }

    MappedFile mapped;
    const auto length = static_cast<std::size_t>(info.st_size);
    if (length == 0) {
        ::close(fd);
        return mapped;
    }

    void* address = mmap(nullptr, length, PROT_READ, MAP_PRIVATE, fd, 0);
    ::close(fd);
    if (address == MAP_FAILED) {
        return std::nullopt;
    }

    mapped.data_ = static_cast<const unsigned char*>(address);
    mapped.size_ = length;
    return mapped;
}
