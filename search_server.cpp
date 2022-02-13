#include "search_server.h"
#include "iterator_range.h"
#include "duration.h"
#include "parse.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>

vector<string> SplitIntoWords(const string& line) {
    istringstream words_input(line);
    return {istream_iterator<string>(words_input), istream_iterator<string>()};
}

SearchServer::SearchServer(istream& document_input) {
    UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream& document_input) {
    TotalDuration totalDuration("UpdateDocumentBase");
    ADD_DURATION(totalDuration);
    InvertedIndex new_index;

    for (string current_document; getline(document_input, current_document); ) {
        new_index.Add(move(current_document));
    }

    index = move(new_index);
}

void SearchServer::AddQueriesStream(
        istream& query_input, ostream& search_results_output
) {
    TotalDuration totalDuration("AddQueriesStream");
    ADD_DURATION(totalDuration);
    vector<size_t> docid_count(index.count(), 0);
    vector<size_t> docids;
    docids.reserve(index.count());
    for (string current_query; getline(query_input, current_query); ) {
        const auto words = SplitBySv(current_query);
        for (const auto& word : words) {
            for (const size_t docid : index.Lookup(word)) {
                if(docid_count[docid] == 0) docids.push_back(docid);
                docid_count[docid]++;
            }
        }
        vector<pair<size_t, size_t>> search_results;
        search_results.reserve(docids.size());
        for(size_t docid : docids){
            search_results.push_back({docid, docid_count[docid]});
            docid_count[docid] = 0;
        }
        partial_sort(begin(search_results), begin(search_results) + min<size_t>(5, search_results.size()), end(search_results), [](const auto& lhs, const auto& rhs){
            return (lhs.second == rhs.second && lhs.first < rhs.first) || lhs.second > rhs.second;
        });
        search_results_output << current_query << ':';
        for (auto [docid, hitcount] : Head(search_results, 5)) {
            search_results_output << " {"
                                  << "docid: " << docid << ", "
                                  << "hitcount: " << hitcount << '}';
        }
        search_results_output << endl;
        docids.clear();
    }
}

void InvertedIndex::Add(string document) {
    docs.push_back(move(document));

    const size_t docid = docs.size() - 1;
    for (const auto& word : SplitBySv(docs.back())) {
        index[word].push_back(docid);
    }
}

const list<size_t>& InvertedIndex::Lookup(const string& word) const {
    if (auto it = index.find(word); it != index.end()) {
        return it->second;
    } else {
        return empty;
    }
}
