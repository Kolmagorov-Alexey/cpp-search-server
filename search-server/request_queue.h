#pragma once
#include <string>
#include <vector>
#include <deque>
#include "search_server.h"
#include "document.h"
 
class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);
 
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
 
private:
    struct QueryResult {
        int req;
        uint64_t timestamp;
    };
    std::deque<QueryResult> requests_;
    const SearchServer& search_server_;
    int count_request_ = 0;
    uint64_t current_time_ = 0;
    const static int min_in_day_ = 1440;
 
 
    void AddRequest(int num_request);
        
};
 
template<typename DocumentPredicate>
inline std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate)
{
    const auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
    AddRequest(result.size());
    return result;
}