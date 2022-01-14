#include "search_server.h"

#include <stdexcept>
#include <execution>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <string_view>

SearchServer::SearchServer(const std::string_view& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

void SearchServer::AddDocument(int document_id, const std::string_view& document, DocumentStatus status, const std::vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0)) {
    	throw std::invalid_argument("document id is negative or already exists");
    }
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);

    for (const std::string& word : words) {
    	if (IsValidWord(word)){
        	word_to_document_freqs_[word][document_id] += 1.0 / words.size();
            document_to_word_freqs_[document_id][word] += 1.0 / words.size();
    	}else{
    		RemoveDocument(document_id);
    		throw std::invalid_argument("Word has illegal characters");
    	}
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
    	return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

std::map<std::string_view, double> word_freqs_;
const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
	word_freqs_.clear();
	if (document_to_word_freqs_.count(document_id) == 1){
		for (const auto& el : document_to_word_freqs_.at(document_id)){;
			const std::string_view el_first_sv = el.first;
			word_freqs_.emplace(el_first_sv, el.second);
		}
	}
	return word_freqs_;
}

void SearchServer::RemoveDocument(int document_id){
	return RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy& policy, int document_id){
    if (document_ids_.count(document_id) != 0){
        document_ids_.erase(document_id);
        documents_.erase(document_id);

        const auto word_to_freqs = document_to_word_freqs_.at(document_id);
        std::for_each(
            policy,
			word_to_freqs.begin(),
			word_to_freqs.end(),
            [this, document_id](const auto& p)
            {word_to_document_freqs_.at(p.first).erase(document_id);}
        );

        document_to_word_freqs_.erase(document_id);
    }
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy& policy, int document_id){
    if (document_ids_.count(document_id) != 0){
        document_ids_.erase(document_id);
        documents_.erase(document_id);

        const auto word_to_freqs = document_to_word_freqs_.at(document_id);
        std::for_each(
            policy,
			word_to_freqs.begin(),
			word_to_freqs.end(),
            [this, document_id](const auto& p)
            {word_to_document_freqs_.at(p.first).erase(document_id);}
        );

        document_to_word_freqs_.erase(document_id);
    }
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy& policy ,const std::string_view& raw_query, int document_id) const {
	const Query query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;
    for (const std::string_view& word : query.plus_words) {
    	if (word_to_document_freqs_.count({word.begin(), word.end()}) == 0) {
    		continue;
    	}
    	if (word_to_document_freqs_.at({word.begin(), word.end()}).count(document_id)) {
    	    matched_words.push_back(word);
    	}
    }
    for (const std::string_view& word : query.minus_words) {
    	if (word_to_document_freqs_.count({word.begin(), word.end()}) == 0) {
    		continue;
    	}
    	if (word_to_document_freqs_.at({word.begin(), word.end()}).count(document_id)) {
    	    matched_words.clear();
    	    break;
    	}
    }

    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy& policy ,const std::string_view& raw_query, int document_id) const {
	const Query query = ParseQuery(raw_query);
    std::vector<std::string_view> matched_words;
    std::mutex mutex_;

	std::for_each(
			std::execution::par,
			query.plus_words.begin(), query.plus_words.end(),
					[&](const std::string_view& word){
    	if (word_to_document_freqs_.at({word.begin(), word.end()}).count(document_id)) {
    		std::lock_guard guard(mutex_);
    	    matched_words.push_back(word);
    	}
	});

	std::for_each(
			std::execution::par,
			query.minus_words.begin(), query.minus_words.end(),
					[&](const std::string_view& word){
    	if (word_to_document_freqs_.at({word.begin(), word.end()}).count(document_id)) {
    		std::lock_guard guard(mutex_);
    	    matched_words.clear();
    	}
	});

    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view& raw_query, int document_id) const {
	return MatchDocument(std::execution::seq, raw_query, document_id);
}

bool SearchServer::IsStopWord(const std::string_view& word) const {
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const std::string_view& word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string_view& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)){
    	if (!IsStopWord(word)) {
    		words.push_back(word);
    	}
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    if (text[0] == '-') {
    	is_minus = true;
    	text = text.substr(1);
    }
    if (text.empty() || text[0] == '-'){
    	throw std::invalid_argument("Wrong value of minus word");
    }
    return { text, is_minus, IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view& text) const {
    Query query;
    for (const std::string_view& word : SplitIntoWordsView(text)){
    	if (IsValidWord(word)){
    		const QueryWord query_word = ParseQueryWord(word);
    		if (query_word.is_stop){
    			continue;
    		}
    		if (query_word.is_minus){
    			query.minus_words.insert(query_word.data);
    		}else{
    			query.plus_words.insert(query_word.data);
    		}
    	}else{
    		throw std::invalid_argument("Word has illegal characters");
    	}
    }
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return std::log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
