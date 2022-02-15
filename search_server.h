#pragma once

#include "Synchronized.h"
#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <deque>
#include <future>


class InvertedIndex {
public:
    struct Entry{
        size_t docid, hitcount;
    };
    InvertedIndex() = default;
    explicit InvertedIndex(std::istream& document_input);
    const std::vector<Entry> & Lookup(std::string_view word) const;
    const std::deque<std::string>& GetDocuments() const {
        return docs;
    }
private:
    std::deque<std::string> docs;
    std::unordered_map<std::string_view, std::vector<Entry>> index;
};

class SearchServer {
public:
    SearchServer() = default;
    explicit SearchServer(std::istream& document_input) : index(InvertedIndex(document_input)) {}
    void UpdateDocumentBase(std::istream& document_input);
    void AddQueriesStream(std::istream& query_input, std::ostream& search_results_output);

private:
    Synchronized<InvertedIndex> index;
    std::vector<std::future<void>> async_tasks;
};
