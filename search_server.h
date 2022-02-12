#pragma once

#include <istream>
#include <ostream>
#include <set>
#include <list>
#include <vector>
#include <map>
#include <string>
#include <future>
using namespace std;

class InvertedIndex {
public:
    void Add(const string& document);
    const list<size_t>&  Lookup(const string& word) const;

    const string& GetDocument(size_t id) const {
        return docs[id];
    }
    size_t size() const{
        return docs.size();
    }
private:
    map<string, list<size_t>> index;
    vector<string> docs;
    list<size_t> empty;
};

class SearchServer {
public:
    SearchServer() = default;
    explicit SearchServer(istream& document_input);
    void UpdateDocumentBase(istream& document_input);
    void AddQueriesStream(istream& query_input, ostream& search_results_output);
    void ParseQuery(const string& current_query, vector<size_t>& docid_count, stringstream& search_results_output);
    stringstream ParseQueries(vector<string> queries);
private:
    InvertedIndex index;
    const int max_count_docs_on_thread = 10000;
    mutex m;
};

lock_guard<mutex> lock(mutex& m);
