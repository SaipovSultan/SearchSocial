#include "search_server.h"
#include "iterator_range.h"
#include "parse.h"

#include <algorithm>
#include <iostream>
#include <numeric>

InvertedIndex::InvertedIndex(std::istream& document_input) {
    for (std::string current_document; getline(document_input, current_document); ) {
        docs.push_back(std::move(current_document));
        const size_t docid = docs.size() - 1;
        for (std::string_view word : SplitIntoWords(docs.back())) {
            auto& docids= index[word];
            if(docids.empty() || docids.back().docid != docid){
                docids.push_back({docid, 1u});
            }else{
                ++docids.back().hitcount;
            }
        }
    }
}

const std::vector<InvertedIndex::Entry>& InvertedIndex::Lookup(std::string_view word) const {
    static const std::vector<InvertedIndex::Entry> empty;
    if (auto it = index.find(word); it != index.end()) {
        return it->second;
    }
    else {
        return empty;
    }
}

void UpdateIndex(std::istream& document_input, Synchronized<InvertedIndex>& index) {
    InvertedIndex new_index(document_input);
    std::swap(index.GetAccess().ref_to_value, new_index);
}

void SearchServer::UpdateDocumentBase(std::istream& document_input) {
    async_tasks.push_back(async(UpdateIndex, std::ref(document_input), std::ref(index)));
    async_tasks.back().get();
}

void ProcessSearches(
        std::istream& query_input, std::ostream& search_results_output, Synchronized<InvertedIndex>& index_handle
) {
    std::vector<size_t> docid_count;
    std::vector<size_t> docids;

    for (std::string current_query; getline(query_input, current_query); )
    {
        const auto words = SplitIntoWords(current_query);
        {
            auto access = index_handle.GetAccess();
            const size_t doc_count = access.ref_to_value.GetDocuments().size();
            docid_count.assign(doc_count, 0);
            docids.resize(doc_count);
            auto& index = access.ref_to_value;

            for (const auto& word : words) {
                for (const auto& [docid, hit_count] : index.Lookup(word)) {
                    docid_count[docid] += hit_count;
                }
            }
        }

        std::iota(docids.begin(), docids.end(), 0u);

        partial_sort(
                docids.begin(),
                Head(docids, 5).end(),
                docids.end(),
                [&docid_count](const size_t& lhs, const size_t& rhs) {
                    return docid_count[lhs] > docid_count[rhs] || (docid_count[lhs] == docid_count[rhs] && lhs < rhs);
                }
        );

        search_results_output << current_query << ':';
        for (size_t docid : Head(docids, 5))
        {
            const size_t hitcount = docid_count[docid];
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
    async_tasks.push_back(
            async(
                    ProcessSearches, std::ref(query_input),
                    std::ref(search_results_output), std::ref(index)
            )
    );
}