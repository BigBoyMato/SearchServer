#pragma once

#include "string_processing.h"
#include "document.h"
#include "concurrent_map.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <vector>
#include <string>
#include <stdexcept>
#include <string_view>
#include <execution>
#include <utility>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
    {
        for (const auto& stop_word : stop_words_){
            if (!IsValidWord(stop_word)){
                throw std::invalid_argument("stop words are invalid");
            }
        }
    }

    explicit SearchServer(const std::string_view& stop_words_text);

    explicit SearchServer(const std::string& stop_words_text);

    void AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename DocumentPredicate, typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy,
    		const std::string_view& raw_query, DocumentPredicate document_predicate) const {
    	std::vector<Document> matched_documents;
        const Query query = ParseQuery(raw_query);

        matched_documents = FindAllDocuments(std::forward<ExecutionPolicy>(policy), query, document_predicate);
        std::sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentPredicate document_predicate) const {
    	return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
    }

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query, DocumentStatus status) const {
        return FindTopDocuments(std::forward<ExecutionPolicy>(policy), raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        	return document_status == status;
        });
    }

    template <typename ExecutionPolicy>
    std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, const std::string_view& raw_query) const {
    	 return FindTopDocuments(std::forward<ExecutionPolicy>(policy), raw_query, DocumentStatus::ACTUAL);
    }

    std::vector<Document> FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view& raw_query) const;
    int GetDocumentCount() const;
    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy& policy, int document_id);
    void RemoveDocument(const std::execution::parallel_policy& polic, int document_id);

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy& policy,
    			const std::string_view& raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy& policy,
    			const std::string_view& raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view& raw_query, int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };
    struct Query {
        std::set<std::string_view> plus_words;
        std::set<std::string_view> minus_words;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string, std::map<int, double>> word_to_document_freqs_;
    std::map<int, std::map<std::string, double>> document_to_word_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(const std::string_view& word) const;
    static bool IsValidWord(const std::string_view& word);
    std::vector<std::string> SplitIntoWordsNoStop(const std::string_view& text) const;
    static int ComputeAverageRating(const std::vector<int>& ratings);
    QueryWord ParseQueryWord(std::string_view text) const;
    Query ParseQuery(const std::string_view& text) const;

    double ComputeWordInverseDocumentFreq(const std::string& word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy& policy, const Query& query,
    		DocumentPredicate document_predicate) const {
        std::map<int, double> document_to_relevance;
        for (const std::string_view& word : query.plus_words) {
            if (word_to_document_freqs_.count({word.begin(), word.end()}) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq({word.begin(), word.end()});
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at({word.begin(), word.end()})) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }
        for (const std::string_view& word : query.minus_words) {
            if (word_to_document_freqs_.count({word.begin(), word.end()}) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at({word.begin(), word.end()})) {
                document_to_relevance.erase(document_id);
            }
        }

        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const std::execution::parallel_policy& policy, const Query& query,
    		DocumentPredicate document_predicate) const {

    	ConcurrentMap<int, double> document_to_relevance(4);

    	std::for_each(
    			std::execution::par,
				query.plus_words.begin(), query.plus_words.end(),
						[&](const std::string_view& word){

						if (word_to_document_freqs_.count({word.begin(), word.end()}) > 0){
							const double inverse_document_freq = ComputeWordInverseDocumentFreq({word.begin(), word.end()});

				            for (const auto [document_id, term_freq] : word_to_document_freqs_.at({word.begin(), word.end()})) {
				                const auto& document_data = documents_.at(document_id);
				                if (document_predicate(document_id, document_data.status, document_data.rating)) {
				                	document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
				                }
				            }
						}
    	});

    	std::for_each(
    			std::execution::par,
				query.minus_words.begin(), query.minus_words.end(),
						[&](const std::string_view& word){

    					if (word_to_document_freqs_.count({word.begin(), word.end()}) > 0) {
        					for (const auto [document_id, _] : word_to_document_freqs_.at({word.begin(), word.end()})) {
        						document_to_relevance.erase(document_id);
        					}
    					}

    	});

        std::vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }

        return matched_documents;
    }
};
