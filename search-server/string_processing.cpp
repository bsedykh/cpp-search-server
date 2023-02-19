#include "string_processing.h"

std::vector<std::string_view> SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;
    size_t pos = str.find_first_not_of(" ");
    const size_t pos_end = str.npos;
    while (pos != pos_end) {
        size_t space = str.find(' ', pos);
        result.push_back(space == pos_end ? str.substr(pos) : str.substr(pos, space - pos));
        pos = str.find_first_not_of(" ", space);
    }
    return result;
}

std::unordered_set<std::string_view> UniqueNonEmptyStrings(const std::vector<std::string_view>& strings) {
    std::unordered_set<std::string_view> non_empty_strings;
    for (const auto& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}
