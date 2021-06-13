#include "searcher.h"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>

void Searcher::add_document(const Searcher::Filename &filename, std::istream &strm) {
    int pos = 0;
    std::string str;
    std::unordered_map<std::string, std::unordered_set<int>> positions;
    while (strm >> str) {
        trim(str);
        if (!str.empty()) {
            std::unordered_set<int> ps;
            ps.emplace(pos);
            auto[iter, flag] = positions.emplace(str, ps);
            if (!flag) {
                iter->second.insert(pos);
            }
            pos++;
        }
    }
    std::map<Filename, std::unordered_set<int>> map_t;
    for (auto s : positions) {
        map_t = {{filename, s.second}};
        auto[iter, flag] = map.emplace(s.first, map_t);
        if (!flag) {
            iter->second.emplace(filename, s.second);
        }
    }
    requests.clear();
}

void Searcher::remove_document(const Searcher::Filename &filename) {
    for (auto &st : map) {
        st.second.erase(filename);
    }
    for (auto &p : requests) {
        auto i = p.second.begin();
        while (i != p.second.end()) {
            if (*i == filename) {
                p.second.erase(i);
            } else {
                i++;
            }
        }
    }
}

std::pair<Searcher::DocIterator, Searcher::DocIterator> Searcher::search(const std::string &query) {
    auto iterator = requests.find(query);
    if (iterator != requests.end()) {
        auto vect = iterator->second;
        return (std::make_pair(DocIterator(std::move(vect), 0), DocIterator()));
    }
    std::vector<Filename> vect;
    size_t spos = 0;
    if (spos >= query.size()) {
        throw bqex;
    }
    bool isempty = true;
    while (spos < query.size()) {
        while (!isalpha(query[spos]) && !isdigit(query[spos]) && query[spos] != '\"') {
            spos++;
            if (spos >= query.size()) {
                if (!isempty) {
                    return std::make_pair(DocIterator(std::move(vect), 0), DocIterator());
                } else {
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

std::vector<Searcher::Filename> Searcher::search_ordered(std::string &&query) {
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
    } else {
        throw bqex;
    }
    while (ss >> a) {
        size_t iter = 0;
        trim(a);
        if (!a.empty()) {
            auto map_it = map.find(a);
            if (map_it == map.end()) {
                vect.clear();
                ins(std::move(query), vect);
                return vect;
            }
            const auto &map_doc = map_it->second;
            for (const auto &i : map_doc) {
                const std::string &i_f = i.first;
                const auto &set = i.second;
                while (iter != vect.size() && vect[iter] < i_f) {
                    vect.erase(vect.begin() + iter);
                    if (vect.empty()) {
                        vect.clear();
                        ins(std::move(query), vect);
                        return vect;
                    }
                    if (iter == vect.size()) {
                        break;
                    }
                }
                if (iter != vect.size() && vect[iter] == i_f) {
                    auto it_ind = indexes.find(vect[iter]);
                    auto &set_ind = it_ind->second;
                    std::unordered_set<int> temp_inds;
                    for (const auto &ind : set_ind) {
                        if (set.find(ind + 1) != set.end()) {
                            temp_inds.insert(ind + 1);
                        }
                    }
                    set_ind = std::move(temp_inds);
                    if (set_ind.empty()) {
                        vect.erase(vect.begin() + iter);
                        if (vect.empty()) {
                            vect.clear();
                            ins(std::move(query), vect);
                            return vect;
                        }
                    } else {
                        iter++;
                    }
                }
                if (iter == vect.size()) {
                    break;
                }
            }
            if (iter != vect.size()) {
                vect.erase(vect.begin() + iter, vect.end());
                if (vect.empty()) {
                    vect.clear();
                    ins(std::move(query), vect);
                    return vect;
                }
            }
        }
    }
    ins(std::move(query), vect);
    return vect;
}

std::vector<Searcher::Filename> Searcher::search_unordered(std::string &&query) {
    if (query.empty()) {
        throw bqex;
    }
    std::unordered_set<Filename> used;
    std::vector<Filename> vect;
    std::stringstream ss(query);
    std::string a;
    std::string a_temp;
    ss >> a;
    trim(a);
    if (!a.empty()) {
        auto map_it = map.find(a);
        if (map_it == map.end()) {
            vect.clear();
            ins(std::move(query), vect);
            return vect;
        }
        const std::map<Filename, std::unordered_set<int>> &map_doc = map_it->second;
        vect.reserve(map_doc.size());
        for (const auto &i : map_doc) {
            vect.push_back(i.first);
        }
        used.insert(a);
    } else {
        throw bqex;
    }
    while (ss >> a_temp) {
        size_t iter = 0;
        trim(a_temp);
        if (used.find(a_temp) != used.end()) {
            continue;
        } else {
            a = a_temp;
        }
        if (!a.empty()) {
            auto map_it = map.find(a);
            if (map_it == map.end()) {
                vect.clear();
                ins(std::move(query), vect);
                return vect;
            }
            const auto &map_doc = map_it->second;
            for (const auto &i : map_doc) {
                const std::string &i_f = i.first;
                while (iter != vect.size() && vect[iter] < i_f) {
                    vect.erase(vect.begin() + iter);
                    if (vect.empty()) {
                        ins(std::move(query), vect);
                        return vect;
                    }
                    if (iter == vect.size()) {
                        break;
                    }
                }
                if (iter != vect.size() && vect[iter] == i_f) {
                    iter++;
                    if (iter == vect.size()) {
                        break;
                    }
                }
                if (iter == vect.size()) {
                    break;
                }
            }
            vect.erase(vect.begin() + iter, vect.end());
            if (vect.empty()) {
                ins(std::move(query), vect);
                return vect;
            }
        } else {
            throw bqex;
        }
    }
    ins(std::move(query), vect);
    return vect;
}

void Searcher::trim(std::string &str) const {
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

bool Searcher::no_more(const std::string &m_query, size_t &spos, std::vector<Filename> &vect) {
    size_t fpos = spos + 1;
    if (m_query[spos] == '\"') {
        while (m_query[fpos] != '\"') {
            fpos++;
            if (fpos >= m_query.size()) {
                throw bqex;
            }
        }
        std::string sub = m_query.substr(spos, fpos - spos + 1);
        auto iterator = requests.find(sub);
        std::vector<Filename> temp_vect;
        if (iterator != requests.end()) {
            temp_vect = iterator->second;
        } else {
            temp_vect = search_ordered(std::move(sub));
        }
        if (temp_vect.empty()) {
            return true;
        } else {
            compare(vect, temp_vect);
        }
        spos = fpos + 1;
    } else {
        while (fpos < m_query.size() && m_query[fpos] != '\"') {
            fpos++;
        }
        std::string sub = m_query.substr(spos, fpos - spos);
        auto iterator = requests.find(sub);
        std::vector<Filename> temp_vect;
        if (iterator != requests.end()) {
            temp_vect = iterator->second;
        } else {
            temp_vect = search_unordered(std::move(sub));
        }
        if (temp_vect.empty()) {
            return true;
        } else {
            compare(vect, temp_vect);
        }
        spos = fpos;
    }
    return vect.empty();
}

void Searcher::precount_o(std::vector<Filename> &temp_vect, const std::string &a,
                          std::unordered_map<Filename, std::unordered_set<int>> &indexes) const {
    auto map_it = map.find(a);
    if (map_it == map.end()) {
        return;
    }
    const std::map<Filename, std::unordered_set<int>> &map_doc = map_it->second;
    temp_vect.reserve(map_doc.size());
    for (const auto &i : map_doc) {
        std::unordered_set<int> temp_set;
        const auto &inds = i.second;
        for (const auto &ii : inds) {
            temp_set.insert(ii);
        }
        indexes.insert(make_pair(i.first, std::move(temp_set)));
        temp_vect.push_back(i.first);
    }
}

void Searcher::compare(std::vector<Filename> &vect, std::vector<Filename> &temp_vect) {
    if (vect.empty()) {
        vect = std::move(temp_vect);
    } else {
        size_t iter = 0;
        for (const auto &i : temp_vect) {
            while (vect[iter] < i) {
                vect.erase(vect.begin() + iter);
                if (iter == vect.size()) {
                    return;
                }
            }
            if (iter == vect.size()) {
                return;
            }
            if (vect[iter] == i) {
                iter++;
                if (iter == vect.size()) {
                    return;
                }
            }
        }
        vect.erase(vect.begin() + iter, vect.end());
    }
}
