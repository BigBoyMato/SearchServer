#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string_view& text) {
    std::vector<std::string> words;
    std::string word;

    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }else{
            word += c;
        }
    }

    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

std::vector<std::string_view> SplitIntoWordsView(std::string_view text) {
    std::vector<std::string_view> result;
    while (true) {
        const size_t space = text.find(' ', 0);
        result.push_back(space == text.npos ? text.substr(0) : text.substr(0, space));
        if (space == text.npos) {
            break;
        } else {
        	text.remove_prefix(space+1);
        }
    }

    return result;
}
