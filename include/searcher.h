#pragma once

#include <string>
#include <utility>

class Searcher
{
public:
    using Filename = std::string; // or std::filesystem::path

    // index modification
    void add_document(const Filename & filename, std::istream & strm);

    void remove_document(const Filename & filename);

    // queries
    class DocIterator
    {
    public:
        using value_type = const Filename;
        ...
    };

    class BadQuery : public std::exception
    {
        ...
    };

    std::pair<DocIterator, DocIterator> search(const std::string & query) const;
};
