#include "search_server.h"
#include "iterator_range.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
#include <future>

vector<string> SplitIntoWords(const string& line) {
    istringstream words_input(line);
    return {istream_iterator<string>(words_input), istream_iterator<string>()};
}

SearchServer::SearchServer(istream& document_input) {
    UpdateDocumentBase(document_input);
}

void SearchServer::UpdateDocumentBase(istream& document_input) {
    InvertedIndex new_index;

    for (string current_document; getline(document_input, current_document); ) {
        new_index.Add(move(current_document));
    }

    index = move(new_index);
}

string SearchServer::ParseQuery(const string& query) {
    const auto words = SplitIntoWords(query);

    map<size_t, size_t> docid_count;
    for (const auto& word : words) {
        for (const size_t docid : index.Lookup(word)) {
            docid_count[docid]++;
        }
    }

    vector<pair<size_t, size_t>> search_results(
            docid_count.begin(), docid_count.end()
    );
    sort(
            begin(search_results),
            end(search_results),
            [](pair<size_t, size_t> lhs, pair<size_t, size_t> rhs) {
                int64_t lhs_docid = lhs.first;
                auto lhs_hit_count = lhs.second;
                int64_t rhs_docid = rhs.first;
                auto rhs_hit_count = rhs.second;
                return make_pair(lhs_hit_count, -lhs_docid) > make_pair(rhs_hit_count, -rhs_docid);
            }
    );
    stringstream search_results_output;
    search_results_output << query << ':';
    for (auto [docid, hitcount] : Head(search_results, 5)) {
        if(hitcount == 0) continue;
        search_results_output << " {"
                              << "docid: " << docid << ", "
                              << "hitcount: " << hitcount << '}';
    }
    search_results_output << endl;
    return search_results_output.str();
}

vector<string> SearchServer::ParseQueries(vector<string> queries){
    vector<string> result;
    result.reserve(queries.size());
    for(const string& query : queries){
        result.push_back(ParseQuery(query));
    }
    return result;
}

void SearchServer::AddQueriesStream(
        istream& query_input, ostream& search_results_output
) {
    size_t order = 0;
    vector<string> queries;
    queries.reserve(block_size);
    vector<future<pair<size_t, vector<string>>>> futures;
    for (string current_query; getline(query_input, current_query); ) {
        queries.push_back(move(current_query));
        if(queries.size() >= block_size){
            futures.push_back(async([&queries, this, order]{
                return pair<size_t, vector<string>>{order, ParseQueries(move(queries))};
            }));
            queries.reserve(block_size);
            ++order;
        }
    }
    if(!queries.empty()) {
        futures.push_back(async([&queries, this, order]{
            return pair<size_t, vector<string>>{order, ParseQueries(move(queries))};
        }));
    }

    vector<pair<size_t, vector<string>>> blocks;
    for(auto& future : futures){
        blocks.push_back(future.get());
    }

    sort(begin(blocks), end(blocks), [](const auto& lhs, const auto& rhs){
        return lhs.first < rhs.first;
    });

    for(const auto& results : blocks){
        for(const auto& result : results.second){
            search_results_output << result;
        }
    }
}

void InvertedIndex::Add(const string& document) {
    docs.push_back(document);

    const size_t docid = docs.size() - 1;
    for (const auto& word : SplitIntoWords(document)) {
        index[word].push_back(docid);
    }
}

list<size_t> InvertedIndex::Lookup(const string& word) const {
    if (auto it = index.find(word); it != index.end()) {
        return it->second;
    } else {
        return {};
    }
}
