#include "request_queue.h"
 
 
RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server), count_request_(0)
{
}
 
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status)
{
    const auto result = search_server_.FindTopDocuments(raw_query, status);
    AddRequest(result.size());
    return result;
}
 
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query)
{
    const auto result = search_server_.FindTopDocuments(raw_query);
    AddRequest(result.size());
    return result;
}
 
int RequestQueue::GetNoResultRequests() const
{
    return min_in_day_ - count_request_;
}
 
void RequestQueue::AddRequest(int num_request)
{
    if (num_request != 0)
    {
        ++current_time_;
        ++count_request_;
        requests_.push_back({ num_request, current_time_ });
    }
}