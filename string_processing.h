#pragma once

#include <vector>
#include <set>
#include <string>
#include <string_view>

std::vector<std::string> SplitIntoWords(const std::string_view& text);

std::vector<std::string_view> SplitIntoWordsView(std::string_view text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (const auto& s : strings) {
        if (!s.empty()) {
            non_empty_strings.insert(s);
        }
    }
    return non_empty_strings;
}
