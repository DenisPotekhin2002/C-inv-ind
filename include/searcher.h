#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <list>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

class Searcher {
public:
    using Filename = std::string; // or std::filesystem::path

    // index modification
    void add_document(const Filename &filename, std::istream &strm);

    void remove_document(const Filename &filename);

    // queries
    class DocIterator {
    private:
        std::vector<Filename> vect;
        size_t i;

    public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = const Filename;
        using pointer = const value_type *;
        using reference = const value_type &;

        DocIterator(std::vector<Filename> &&v, size_t ind)
                : vect(std::move(v)), i(ind) {};

        DocIterator()
                : i(0) {};

        DocIterator &operator++() {
            if (*this != DocIterator()) {
                ++i;
                if (i == vect.size()) {
                    *this = DocIterator();
                }
            }
            return *this;
        };

        DocIterator operator++(int) {
            auto temp = *this;
            ++*this;
            return temp;
        };

        friend bool operator==(const DocIterator &iter1, const DocIterator &iter2) {
            return iter1.vect == iter2.vect && iter1.i == iter2.i;
        };

        friend bool operator!=(const DocIterator &iter1, const DocIterator &iter2) { return !(iter1 == iter2); };

        pointer operator->() const { return &vect[i]; };

        reference operator*() const {
            return vect[i];
        }
    };

    class BadQuery : public std::exception {
        virtual const char *what() const throw() override {
            return "Search query syntax error: bad query";
        }
    } bqex;

    std::pair<DocIterator, DocIterator> search(const std::string &query);

    std::vector<Filename> search_ordered(std::string &&);

    std::vector<Filename> search_unordered(std::string &&query);

    void ins(std::string &&query, std::vector<Filename> &vect) {
        //std::cout << "ins " << query << std::endl;
        requests.emplace(std::move(query), vect);
    }

private:
    std::unordered_map<std::string, std::map<Filename, std::unordered_set<int>>> map;
    std::unordered_map<std::string, std::vector<Filename>> requests;

    void trim(std::string &str) const;

    bool no_more(const std::string &m_query, size_t &spos, std::vector<Filename> &vector);

    void precount_o(std::vector<Filename> &temp_vect, const std::string &a,
                    std::unordered_map<Filename, std::unordered_set<int>> &indexes) const;

    void compare(std::vector<Filename> &vect, std::vector<Filename> &temp_vect);
};
