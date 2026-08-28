#include "frontier.hpp"

bool Frontier::push(const Url& url, std::size_t depth) {
    if (depth > max_depth_) {
        return false;
    }
    if (seen_.size() >= max_pages_ && seen_.find(url_to_string(url)) == seen_.end()) {
        return false;
    }

    const auto [it, inserted] = seen_.insert(url_to_string(url));
    if (!inserted) {
        return false;
    }

    queue_.push_back(FrontierEntry{url, depth});
    return true;
}

std::optional<FrontierEntry> Frontier::pop() {
    if (queue_.empty() || issued_ >= max_pages_) {
        return std::nullopt;
    }

    FrontierEntry entry = queue_.front();
    queue_.pop_front();
    issued_++;
    return entry;
}
