#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "concurrent_map.h"
#include "search_server.h"
#include "string_processing.h"

SearchServer::SearchServer(const std::string& stop_words) {
	const auto stop_words_sv = UniqueNonEmptyStrings(SplitIntoWords(stop_words));
	if (!std::all_of(stop_words_sv.begin(), stop_words_sv.end(), IsValidWord)) {
		throw std::invalid_argument("Some of stop words are invalid");
	}

	for (const auto& stop_word_sv : stop_words_sv) {
		stop_words_.insert(std::string(stop_word_sv));
	}
}

void SearchServer::AddDocument(DocumentId document_id, const std::string& document) {
	if (documents_.count(document_id) > 0) {
		throw std::invalid_argument("Invalid document_id");
	}

	const auto [it, _] = documents_.emplace(document_id, document);

	const auto words_sv = SplitIntoWordsNoStop(it->second);
	const double inv_word_count = 1.0 / words_sv.size();
	for (const auto& word_sv : words_sv) {
		word_to_document_freqs_[word_sv][document_id] += inv_word_count;
	}
}

std::vector<SearchServer::Result> SearchServer::Find(const std::string& raw_query, size_t results_num) const {
	return Find(std::execution::seq, raw_query, results_num);
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(std::string_view text) const {
	using namespace std::string_literals;
	std::vector<std::string_view> words;
	for (const auto& word : SplitIntoWords(text)) {
		if (!IsValidWord(word)) {
			throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
		}
		if (!IsStopWord(word)) {
			words.push_back(word);
		}
	}
	return words;
}

bool SearchServer::IsValidWord(std::string_view word) {
	return std::none_of(word.begin(), word.end(), [](char c) {
			return c >= '\0' && c < ' ';
		});
}

bool SearchServer::IsStopWord(std::string_view word) const {
	return stop_words_.count(std::string(word)) > 0;
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
	std::unordered_set<std::string_view> plus_words;
	std::unordered_set<std::string_view> minus_words;
	for (const auto& word : SplitIntoWords(text)) {
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
			if (query_word.is_minus) {
				minus_words.insert(query_word.data);
			}
			else {
				plus_words.insert(query_word.data);
			}
		}
	}
	return { std::vector(plus_words.begin(), plus_words.end()), 
		     std::vector(minus_words.begin(), minus_words.end()) };
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view word) const {
	using namespace std::string_literals;

	if (word.empty()) {
		throw std::invalid_argument("Query word is empty"s);
	}

	bool is_minus = false;
	if (word[0] == '-') {
		is_minus = true;
		word = word.substr(1);
	}
	if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
		throw std::invalid_argument("Query word "s + std::string(word) + " is invalid");
	}

	return { word, is_minus, IsStopWord(word) };
}

std::unordered_map<SearchServer::DocumentId, double> SearchServer::FindDocumentsWithPlusWords(const std::execution::sequenced_policy& policy, const Query& query) const {
	std::unordered_map<DocumentId, double> result;

	std::for_each(
		policy,
		query.plus_words.begin(),
		query.plus_words.end(),
		[this, &result](const auto& word) {
			const auto word_iterator = word_to_document_freqs_.find(word);
			if (word_iterator == word_to_document_freqs_.end()) {
				return;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			for (const auto [document_id, term_freq] : word_iterator->second) {
				result[document_id] += term_freq * inverse_document_freq;
			}
		}
	);

	return result;
}

std::unordered_map<SearchServer::DocumentId, double> SearchServer::FindDocumentsWithPlusWords(const std::execution::parallel_policy& policy, const Query& query) const {
	const size_t bucket_count = 100;
	ConcurrentMap<DocumentId, double> result(bucket_count);

	std::for_each(
		policy,
		query.plus_words.begin(),
		query.plus_words.end(),
		[this, &result](const auto& word) {
			const auto word_iterator = word_to_document_freqs_.find(word);
			if (word_iterator == word_to_document_freqs_.end()) {
				return;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			for (const auto [document_id, term_freq] : word_iterator->second) {
				result[document_id].ref_to_value += term_freq * inverse_document_freq;
			}
		}
	);

	return result.BuildOrdinaryMap();
}

double SearchServer::ComputeWordInverseDocumentFreq(std::string_view word) const {
	return log(documents_.size() * 1.0 / word_to_document_freqs_.at(word).size());
}

void SearchServer::EraseDocumentsWithMinusWords(std::unordered_map<DocumentId, double>& document_to_relevance, const std::vector<std::string_view>& minus_words) const {
	for (const auto& word : minus_words) {
		const auto word_iterator = word_to_document_freqs_.find(word);
		if (word_iterator == word_to_document_freqs_.end()) {
			continue;
		}
		for (const auto [document_id, _] : word_iterator->second) {
			document_to_relevance.erase(document_id);
		}
	}
}

std::vector<SearchServer::Result> SearchServer::ConvertToResults(const std::unordered_map<DocumentId, double>& document_to_relevance) const {
	std::vector<Result> results;
	results.reserve(document_to_relevance.size());
	for (const auto [document_id, relevance] : document_to_relevance) {
		results.push_back({ document_id, relevance });
	}
	return results;
}

void SearchServer::LeaveTopResults(std::vector<Result>& documents, size_t results_num) {
	sort(
		documents.begin(), documents.end(),
		[](const Result& lhs, const Result& rhs) {
			return lhs.relevance > rhs.relevance;
		}
	);
	if (documents.size() > results_num) {
		documents.resize(results_num);
	}
}
