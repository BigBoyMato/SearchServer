#pragma once

#include <vector>
#include <iostream>

template <typename Iterator>
class IteratorRange {
    public:
        IteratorRange(Iterator first, Iterator last)
        	: first_(first), last_(last)
        {
        }

        Iterator begin() const {
            return first_;
        }

        Iterator end() const {
            return last_;
        }

        size_t size() const {
            return std::distance(first_, last_);
        }

    private:
        Iterator first_, last_;
};

template <typename Iterator>
class Paginator {
	public:
        Paginator(Iterator first, Iterator last, size_t page_size)
        {
        	for (auto i = first; i != last; std::advance(i, page_size)){
        		if (std::distance(i, last) > page_size){
        			pages.emplace_back(i, std::next(i, page_size));
        	    }else{
        	    	pages.emplace_back(i, last);
        	        break;
        	    }
        	}
        }
        auto begin() const {
            return pages.begin();
        }

       auto end() const {
            return pages.end();
        }

        size_t size() const {
            return pages.size();
        }

    private:
        std::vector<IteratorRange<Iterator>> pages;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
	return Paginator(begin(c), end(c), page_size);
}

template <typename Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator>& iterator_range){
	for (const auto& content : iterator_range){
		os << content;
	}
	return os;
}
