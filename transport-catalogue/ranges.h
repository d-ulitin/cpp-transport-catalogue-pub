#pragma once

#include <iterator>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace ranges {

template <typename It>
class Range {
public:
    using ValueType = typename std::iterator_traits<It>::value_type;

    Range(It begin, It end)
        : begin_(begin)
        , end_(end) {
    }

    It begin() const {
        return begin_;
    }
    It end() const {
        return end_;
    }

private:
    It begin_;
    It end_;
};

template <typename C>
auto AsRange(const C& container) {
    return Range{container.begin(), container.end()};
}

template <typename It>
auto AsRange(std::pair<It, It> p) {
    return Range{p.first, p.second};
}

}  // namespace ranges