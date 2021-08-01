#include "test_example_functions.h"

#include <iostream>

void AddDocument(SearchServer& search_server, int document_id, const std::string_view& document, DocumentStatus status,
                 const std::vector<int>& ratings) {
    try {
    	search_server.AddDocument(document_id, document, status, ratings);
    } catch (const std::exception& e) {
        std::cout << "Document addition error " << document_id << ": " << e.what() << std::endl;
    }
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words, DocumentStatus status) {
    std::cout << "{ "
    		<< "document_id = " << document_id << ", "
			<< "status = " << static_cast<int>(status) << ", "
			<< "words =";
    for (const auto& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}" << std::endl;
}

void PrintDocument(const Document& document) {
	std::cout << document << std::endl;
}

void FindTopDocuments(const SearchServer& search_server, const std::string_view& raw_query) {
    std::cout << "Search results on query: " << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const std::exception& e) {
        std::cout << "Search error: " << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string_view& query) {
    try {
        std::cout << "Document matching on query: " << query << std::endl;
        for (const auto& document_id : search_server) {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const std::exception& e) {
        std::cout << "Document matching error " << query << ": " << e.what() << std::endl;
    }
}
