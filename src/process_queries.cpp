#include "process_queries.h"

#include <execution>
#include <algorithm>

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){
    std::vector<std::vector<Document>> vec_matched_documents(queries.size());
    std::transform(
                std::execution::par,
                queries.begin(), queries.end(),
				vec_matched_documents.begin(),
                [&search_server](std::string_view query){
                    return search_server.FindTopDocuments(query);
                }
            );
    return vec_matched_documents;
}

std::vector<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){
    const auto vec_matched_documents = ProcessQueries(search_server, queries);
    std::vector<Document> matched_documents_flat;
    for (const auto& documents : vec_matched_documents){
    	matched_documents_flat.insert(matched_documents_flat.end(),
    			documents.begin(), documents.end());
    }
    return matched_documents_flat;
}
