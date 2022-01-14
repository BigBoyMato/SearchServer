#include "remove_duplicates.h"

#include <string>
#include <set>
#include <map>
#include <iostream>
#include <vector>

void RemoveDuplicates(SearchServer& search_server){
    std::map<std::set<std::string>, int> document_id_to_words;
    std::vector<int> ids_to_remove;

    for (const auto& document_id : search_server){
    	std::set<std::string> words;
        for (const auto& [word, _] : search_server.GetWordFrequencies(document_id)){
        	words.insert({word.begin(), word.end()});
        }
        if (!document_id_to_words.try_emplace(words, document_id).second){
            ids_to_remove.push_back(document_id);
        }
    }
    for (auto id : ids_to_remove){
        std::cout << "Found duplicate document id " << id << std::endl;
        search_server.RemoveDocument(id);
    }
}

