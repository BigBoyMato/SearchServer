#include "request_queue.h"

RequestQueue::RequestQueue(const SearchServer& search_server) :
    search_server_(search_server)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    const auto documents = search_server_.FindTopDocuments(raw_query, status);
    UpdateRequests(documents.empty());
    return documents;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    const auto documents = search_server_.FindTopDocuments(raw_query);
    UpdateRequests(documents.empty());
    return documents;
}

int RequestQueue::GetNoResultRequests() const {
    return no_result_requests_count;
}

void RequestQueue::UpdateRequests(bool is_last_result_empty){
    timestamp++;
    QueryResult query_result;
    query_result.timestamp = timestamp;
    query_result.empty = is_last_result_empty;
    requests_.push_back(query_result);
    if (query_result.empty){
    	no_result_requests_count++;
    }
    RemoveOldRequests();
}

void RequestQueue::RemoveOldRequests(){
    if (requests_.empty()){
    	return;
    }
    const long long requests_to_delete_count = (requests_.back().timestamp - requests_.front().timestamp + 1) - sec_in_day_;
    if (requests_to_delete_count <= 0){
    	return;
    }
    for (auto i = requests_to_delete_count; i > 0; --i){
    	if (requests_.front().empty){
    		no_result_requests_count--;
    	}
    	requests_.pop_front();
    }
}
