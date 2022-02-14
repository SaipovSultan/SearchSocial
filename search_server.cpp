#include "search_server.h"
#include "iterator_range.h"
#include "parse.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>
#include <numeric>

void InvertedIndex::Add(std::string&& document) {
    docs.push_back(std::move(document));
    const size_t docid = docs.size() - 1;
    for (std::string_view word : SplitIntoWords(docs.back())) {
        auto& cur_index = index[word];
        if (cur_index.empty() || cur_index.back().first != docid) {
            cur_index.emplace_back(docid, 1u);
        }else{
            cur_index.back().second++;
        }
    }
}

InvertedIndex::InvertedIndex(std::istream& document_input) {
    for (std::string current_document; getline(document_input, current_document); ) {
        Add(move(current_document));
    }
}


void UpdateIndex(std::istream& document_input, Synchronized<InvertedIndex>& index) {
    InvertedIndex new_index(document_input);
    std::swap(index.GetAccess().ref_to_value, new_index);
}

void SearchServer::UpdateDocumentBase(std::istream& document_input) {
    async_threads.push_back(async(UpdateIndex, std::ref(document_input), std::ref(index)));
}


const std::vector<std::pair<size_t, size_t>>& InvertedIndex::Lookup(std::string_view word) const {
    static const std::vector<std::pair<size_t, size_t>> empty;
    if (auto it = index.find(word); it != index.end()) {
        return it->second;
    }
    else {
        return empty;
    }
}


void AddQueriesStreamSingleThread(
        std::istream& query_input, std::ostream& search_results_output, Synchronized<InvertedIndex>& index
) {
    std::vector<size_t> docid_count;
    std::vector<std::pair<size_t, size_t>>search_results;
    for (std::string current_query; getline(query_input, current_query); )
    {
        const auto words = SplitIntoWords(current_query);
        {
            auto access = index.GetAccess();
            const size_t doc_count = access.ref_to_value.GetSize();

            docid_count.assign(doc_count, 0);
            search_results.resize(doc_count);

            auto& index = access.ref_to_value;
            for (const auto& word : words) {
                for (const auto& [docid, hit_count] : index.Lookup(word)) {
                    docid_count[docid] += hit_count;
                }
            }
        }

        for (size_t i = 0; i < search_results.size(); ++i) {
            search_results[i] = { i, docid_count[i] };
        }

        partial_sort(
                search_results.begin(),
                Head(search_results, 5).end(),
                search_results.end(),
                [](std::pair<size_t, size_t> lhs, std::pair<size_t, size_t> rhs) {
                    int64_t lhs_docid = lhs.first;
                    auto lhs_hit_count = lhs.second;
                    int64_t rhs_docid = rhs.first;
                    auto rhs_hit_count = rhs.second;
                    return std::make_pair(lhs_hit_count, -lhs_docid) >
                           std::make_pair(rhs_hit_count, -rhs_docid);
                }
        );

        search_results_output << current_query << ':';
        for (auto [docid, hitcount] : Head(search_results, 5))
        {
            if (hitcount == 0) {
                break;
            }
            search_results_output << " {"
                                  << "docid: " << docid << ", "
                                  << "hitcount: " << hitcount << '}';
        }
        search_results_output << '\n';
    }

}

void SearchServer::AddQueriesStream(
        std::istream& query_input, std::ostream& search_results_output
) {
    async_threads.push_back(
            async(
                    AddQueriesStreamSingleThread, std::ref(query_input),
                    std::ref(search_results_output), std::ref(index)
            )
    );

}