#include "edit_distance.hpp"

#include <algorithm>
#include <vector>

namespace {

std::size_t compute(std::string_view a, std::string_view b, std::size_t limit) {
    if (a.size() > b.size()) {
        std::swap(a, b);
    }

    if (b.size() - a.size() > limit) {
        return limit + 1;
    }

    std::vector<std::size_t> previous(a.size() + 1);
    std::vector<std::size_t> current(a.size() + 1);

    for (std::size_t i = 0; i <= a.size(); i++) {
        previous[i] = i;
    }

    for (std::size_t j = 1; j <= b.size(); j++) {
        current[0] = j;
        std::size_t row_best = current[0];

        for (std::size_t i = 1; i <= a.size(); i++) {
            const std::size_t substitution = previous[i - 1] + (a[i - 1] == b[j - 1] ? 0 : 1);
            const std::size_t deletion = previous[i] + 1;
            const std::size_t insertion = current[i - 1] + 1;

            current[i] = std::min({substitution, deletion, insertion});
            row_best = std::min(row_best, current[i]);
        }

        if (row_best > limit) {
            return limit + 1;
        }

        previous.swap(current);
    }

    return previous[a.size()];
}

}

std::size_t edit_distance(std::string_view a, std::string_view b) {
    return compute(a, b, a.size() + b.size());
}

std::size_t bounded_edit_distance(std::string_view a, std::string_view b, std::size_t limit) {
    return compute(a, b, limit);
}
