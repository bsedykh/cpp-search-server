#include <execution>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "log_duration.h"
#include "search_server.h"

// Генерирует случайное слово, содержащее символы от 'a' до 'z' в количестве [1, max_length]
std::string GenerateWord(std::mt19937& generator, int max_length) {
    const int length = std::uniform_int_distribution(1, max_length)(generator);
    std::string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        char c = static_cast<char>(std::uniform_int_distribution(static_cast<int>('a'), static_cast<int>('z'))(generator));
        word.push_back(c);
    }
    return word;
}

// Генерирует набор из неповторяющихся слов длины [1, max_length]. размер набора <= word_count
std::vector<std::string> GenerateDictionary(std::mt19937& generator, int word_count, int max_length) {
    std::vector<std::string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

// Генерирует строку поискового запроса, содержащую word_count слов, случайным образом выбранных из словаря dictionary.
// Минус-слова будут добавлены в строку с вероятностью minus_prob.
std::string GenerateQuery(std::mt19937& generator, const std::vector<std::string>& dictionary, int word_count, double minus_prob = 0) {
    std::string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (std::uniform_real_distribution<double>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[std::uniform_int_distribution<size_t>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

// Генерирует набор поисковых запросов размером query_count, содержащих word_count слов
std::vector<std::string> GenerateQueries(std::mt19937& generator, const std::vector<std::string>& dictionary, int query_count, int word_count, double minus_prob = 0) {
    std::vector<std::string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, word_count, minus_prob));
    }
    return queries;
}

template <typename ExecutionPolicy>
void Test(const char* mark, const SearchServer& search_server, const std::vector<std::string>& queries, ExecutionPolicy&& policy) {
    const size_t results_num = 5;

    LOG_DURATION(mark);
    double total_relevance = 0;
    for (const auto& query : queries) {
        for (const auto& document : search_server.Find(policy, query, results_num)) {
            total_relevance += document.relevance;
        }
    }
    std::cout << total_relevance << std::endl;
}

#define TEST(policy) Test(#policy, search_server, queries, std::execution::policy)

int main() {
    std::mt19937 generator;
    const auto dictionary = GenerateDictionary(generator, 100, 10);
    const auto documents = GenerateQueries(generator, dictionary, 100'000, 70);
    SearchServer search_server;
    for (uint32_t i = 0; i < documents.size(); ++i) {
        search_server.AddDocument(i, documents[i]);
    }
    const auto queries = GenerateQueries(generator, dictionary, 100, 70, 0.1);
    TEST(seq);
    TEST(par);
}
