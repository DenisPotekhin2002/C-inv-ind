#include "searcher.h"

#include <algorithm>
#include <cstring>
#include <sstream>

void Searcher::add_document(const Searcher::Filename & filename, std::istream & strm)
{
    int pos = 0;
    std::string str;
    std::unordered_map<std::string, std::unordered_set<int>> positions;
    while (strm >> str) {
        trim(str);
        if (!str.empty()) {
            positions.emplace(str, std::unordered_set<int>({})).first->second.emplace(pos);
            pos++;
        }
    }
    for (const auto & s : positions) {
        map.emplace(s.first, std::map<Filename, std::unordered_set<int>>({})).first->second.emplace(filename, s.second);
    }
    requests.clear();
}

void Searcher::remove_document(const Searcher::Filename & filename)
{
    for (auto & st : map) {
        st.second.erase(filename);
    }
    /*for (auto & p : requests) {
        auto i = p.second.begin();
        while (i != p.second.end()) {
            if (*i == filename) {
                p.second.erase(i);
            }
            else {
                i++;
            }
        }
    }*/
    requests.clear();
}

std::pair<Searcher::DocIterator, Searcher::DocIterator> Searcher::search(const std::string & query)
{
    auto iter = requests.find(query);
    if (iter != requests.end()) {
        return (std::make_pair(DocIterator(iter->second, 0), DocIterator()));
    }
    std::vector<Filename> vect;
    size_t spos = 0;
    if (spos >= query.size()) {
        throw bqex;
    }
    bool isempty = true;
    size_t q_size = query.size();
    while (spos < q_size) {
        while (!isalpha(query[spos]) && !isdigit(query[spos]) && query[spos] != '\"') {
            spos++;
            if (spos >= q_size) {
                if (!isempty) {
                    return std::make_pair(DocIterator(std::move(vect), 0), DocIterator());
                }
                else {
                    throw bqex;
                }
            }
        }
        if (no_more(query, spos, vect)) {
            return std::make_pair(DocIterator(), DocIterator());
        }
        isempty = false;
    }
    return std::make_pair(DocIterator(std::move(vect), 0), DocIterator());
}

std::vector<Searcher::Filename> Searcher::search_ordered(std::string && query)
{
    if (query.empty()) {
        throw bqex;
    }
    std::vector<Filename> vect;
    std::unordered_map<Filename, std::unordered_set<int>> indexes;
    std::stringstream ss(query.substr(1, query.size() - 2));
    std::string a;
    ss >> a;
    trim(a);
    if (!a.empty()) {
        precount_o(vect, a, indexes);
        if (vect.empty()) {
            ins(std::move(query), vect);
            return vect;
        }
    }
    else {
        throw bqex;
    }
    while (ss >> a) {
        auto iter = vect.begin();
        trim(a);
        if (!a.empty()) {
            const auto & map_it = map.find(a);
            if (map_it != map.end()) {
                for (const auto & i : map_it->second) {
                    const std::string & i_f = i.first;
                    auto iter2 = iter;
                    while (iter2 != vect.end() && *iter2 < i_f) {
                        iter2++;
                        if (iter2 == vect.end()) {
                            break;
                        }
                    }
                    vect.erase(iter, iter2);
                    if (vect.empty()) {
                        vect.clear();
                        ins(std::move(query), vect);
                        return vect;
                    }
                    if (iter != vect.end() && *iter == i_f) {
                        auto & set_ind = indexes.find(*iter)->second;
                        std::unordered_set<int> temp_inds;
                        for (const auto & ind : set_ind) {
                            if (i.second.find(ind + 1) != i.second.end()) {
                                temp_inds.emplace(ind + 1);
                            }
                        }
                        set_ind = temp_inds;
                        if (set_ind.empty()) {
                            vect.erase(iter);
                            if (vect.empty()) {
                                vect.clear();
                                ins(std::move(query), vect);
                                return vect;
                            }
                        }
                        else {
                            iter++;
                        }
                    }
                    if (iter == vect.end()) {
                        break;
                    }
                }
                if (iter != vect.end()) {
                    vect.erase(iter, vect.end());
                    if (vect.empty()) {
                        vect.clear();
                        ins(std::move(query), vect);
                        return vect;
                    }
                }
            }
            else {
                vect.clear();
                ins(std::move(query), vect);
                return vect;
            }
        }
    }
    ins(std::move(query), vect);
    return vect;
}

std::vector<Searcher::Filename> Searcher::search_unordered(std::string && query)
{
    if (query.empty()) {
        throw bqex;
    }
    std::vector<Filename> vect;
    std::stringstream ss(query);
    std::string a;
    ss >> a;
    trim(a);
    if (!a.empty()) {
        const auto & map_it = map.find(a);
        if (map_it == map.end()) {
            vect.clear();
            ins(std::move(query), vect);
            return vect;
        }
        const std::map<Filename, std::unordered_set<int>> & map_doc = map_it->second;
        for (const auto & i : map_doc) {
            vect.push_back(i.first);
        }
    }
    else {
        throw bqex;
    }
    while (ss >> a) {
        auto iter = vect.begin();
        trim(a);
        if (!a.empty()) {
            auto it = map.find(a);
            if (it != map.end()) {
                for (const auto & i : it->second) {
                    const std::string & i_f = i.first;
                    auto iter2 = iter;
                    while (iter2 != vect.end() && *iter2 < i_f) {
                        iter2++;
                        if (iter2 == vect.end()) {
                            break;
                        }
                    }
                    vect.erase(iter, iter2);
                    if (vect.empty()) {
                        ins(std::move(query), vect);
                        return vect;
                    }
                    if (iter != vect.end() && *iter == i_f) {
                        iter++;
                        if (iter == vect.end()) {
                            break;
                        }
                    }
                    if (iter == vect.end()) {
                        break;
                    }
                }
                vect.erase(iter, vect.end());
                if (vect.empty()) {
                    ins(std::move(query), vect);
                    return vect;
                }
            }
            else {
                vect.clear();
                ins(std::move(query), vect);
                return vect;
            }
        }
        else {
            throw bqex;
        }
    }
    ins(std::move(query), vect);
    return vect;
}

void Searcher::trim(std::string & str) const
{
    if (str.empty()) {
        return;
    }
    size_t ind1 = 0;
    while (!isalpha(str[ind1]) && !isdigit(str[ind1])) {
        ind1++;
        if (ind1 >= str.size()) {
            str.clear();
            return;
        }
    }
    str.erase(0, ind1);
    int ind2 = str.size() - 1;
    while (!isalpha(str[ind2]) && !isdigit(str[ind2])) {
        ind2--;
        if (ind2 < 0) {
            str.clear();
            return;
        }
    }
    str.erase(ind2 + 1);
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
}

bool Searcher::no_more(const std::string & m_query, size_t & spos, std::vector<Filename> & vect)
{
    size_t fpos = spos + 1;
    if (m_query[spos] == '\"') {
        while (m_query[fpos] != '\"') {
            fpos++;
            if (fpos >= m_query.size()) {
                throw bqex;
            }
        }
        std::string sub = m_query.substr(spos, fpos - spos + 1);
        const auto & iterator = requests.find(sub);
        std::vector<Filename> temp_vect;
        if (iterator != requests.end()) {
            temp_vect = (*iterator).second;
        }
        else {
            temp_vect = search_ordered(std::move(sub));
        }
        if (temp_vect.empty()) {
            return true;
        }
        else {
            compare(vect, std::move(temp_vect));
        }
        spos = fpos + 1;
    }
    else {
        while (fpos < m_query.size() && m_query[fpos] != '\"') {
            fpos++;
        }
        std::string sub = m_query.substr(spos, fpos - spos);
        const auto & iterator = requests.find(sub);
        std::vector<Filename> temp_vect;
        if (iterator != requests.end()) {
            temp_vect = (*iterator).second;
        }
        else {
            temp_vect = search_unordered(std::move(sub));
        }
        if (temp_vect.empty()) {
            return true;
        }
        else {
            compare(vect, std::move(temp_vect));
        }
        spos = fpos;
    }
    return vect.empty();
}

void Searcher::precount_o(std::vector<Filename> & temp_vect, const std::string & a, std::unordered_map<Filename, std::unordered_set<int>> & indexes) const
{
    const auto & map_it = map.find(a);
    if (map_it != map.end()) {
        const std::map<Filename, std::unordered_set<int>> & map_doc = (*map_it).second;
        for (const auto & i : map_doc) {
            std::unordered_set<int> temp_set;
            const auto & inds = i.second;
            for (const auto & ii : inds) {
                temp_set.emplace(ii);
            }
            indexes.emplace(i.first, std::move(temp_set));
            temp_vect.push_back(i.first);
        }
    }
}

void Searcher::compare(std::vector<Filename> & vect, std::vector<Filename> && temp_vect)
{
    if (vect.empty()) {
        vect = std::move(temp_vect);
    }
    else {
        auto iter = vect.begin();
        for (const auto & i : temp_vect) {
            auto iter2 = iter;
            while (*iter2 < i) {
                iter2++;
                if (iter2 == vect.end()) {
                    break;
                }
            }
            vect.erase(iter, iter2);
            if (iter == vect.end()) {
                return;
            }
            if (*iter == i) {
                iter++;
                if (iter == vect.end()) {
                    return;
                }
            }
        }
        vect.erase(iter, vect.end());
    }
}
