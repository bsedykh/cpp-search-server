#pragma once

#include <execution>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class SearchServer {
public:
    using DocumentId = uint32_t;
    
    struct Result {
        DocumentId id;
        double relevance;
    };

    explicit SearchServer() = default;

    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(DocumentId document_id, const std::string& document);

    std::vector<Result> Find(const std::string& raw_query, size_t results_num) const;

    template <typename ExecutionPolicy>
    std::vector<Result> Find(ExecutionPolicy&& policy, const std::string& raw_query, size_t results_num) const;

private:
    std::unordered_set<std::string> stop_words_;
    std::unordered_map<std::string_view, std::unordered_map<DocumentId, double>> word_to_document_freqs_;
    std::unordered_map<DocumentId, std::string> documents_;

    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

    static bool IsValidWord(std::string_view word);

    bool IsStopWord(std::string_view word) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string& text) const;

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(std::string_view text) const;

    template <typename ExecutionPolicy>
    std::vector<Result> FindAll(ExecutionPolicy&& policy, const Query& query) const;

    std::unordered_map<DocumentId, double> FindDocumentsWithPlusWords(const std::execution::sequenced_policy& policy, const Query& query) const;

    std::unordered_map<DocumentId, double> FindDocumentsWithPlusWords(const std::execution::parallel_policy& policy, const Query& query) const;

    double ComputeWordInverseDocumentFreq(std::string_view word) const;

    void EraseDocumentsWithMinusWords(std::unordered_map<DocumentId, double>& document_to_relevance, const std::vector<std::string_view>& minus_words) const;

    std::vector<Result> ConvertToResults(const std::unordered_map<DocumentId, double>& document_to_relevance) const;

    static void LeaveTopResults(std::vector<Result>& documents, size_t results_num);
};

template <typename ExecutionPolicy>
std::vector<SearchServer::Result> SearchServer::Find(ExecutionPolicy&& policy, const std::string& raw_query, size_t results_num) const {
    const auto query = ParseQuery(raw_query);
    auto results = FindAll(std::forward<ExecutionPolicy>(policy), query);
    LeaveTopResults(results, results_num);
    return results;
}

template <typename ExecutionPolicy>
std::vector<SearchServer::Result> SearchServer::FindAll(ExecutionPolicy&& policy, const Query& query) const {
    auto document_to_relevance = FindDocumentsWithPlusWords(std::forward<ExecutionPolicy>(policy), query);
    EraseDocumentsWithMinusWords(document_to_relevance, query.minus_words);
    return ConvertToResults(document_to_relevance);
}
