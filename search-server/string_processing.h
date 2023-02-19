#pragma once

#include <unordered_set>
#include <string_view>
#include <vector>

std::vector<std::string_view> SplitIntoWords(std::string_view text);

std::unordered_set<std::string_view> UniqueNonEmptyStrings(const std::vector<std::string_view>& strings);
