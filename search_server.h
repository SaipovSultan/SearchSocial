#pragma once

#include <istream>
#include <ostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <deque>
#include <future>

#include "Synchronized.h"


class InvertedIndex {
public:
    InvertedIndex() = default;
    explicit InvertedIndex(std::istream& document_input);
    void Add(std::string&& document);
    const std::vector<std::pair<size_t, size_t>>& Lookup(std::string_view word) const;
    const std::deque<std::string>& GetDocuments() const {
        return docs;
    }
    const std::string& GetDocument(size_t id) const {
        return docs[id];
    }
    const size_t GetSize() const {
        return docs.size();
    }
private:
    std::deque<std::string> docs;
    std::unordered_map<std::string_view, std::vector<std::pair<size_t, size_t>>> index;

};

class SearchServer {
public:
    SearchServer() = default;
    explicit SearchServer(std::istream& document_input) :
            index(InvertedIndex(document_input))
    {}
    void UpdateDocumentBase(std::istream& document_input);
    void AddQueriesStream(std::istream& query_input, std::ostream& search_results_output);

private:
    Synchronized<InvertedIndex> index;
    std::vector<std::future<void>> async_threads;
};
