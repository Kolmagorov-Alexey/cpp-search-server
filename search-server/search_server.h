#pragma once
#include <algorithm>
#include <numeric>
#include <map>
#include <vector>
#include <cmath>
#include "read_input_functions.h"
#include "string_processing.h"
#include "document.h"
#include "paginator.h"
 
using namespace std;
 
const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;
 
class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
  
    explicit SearchServer(const string& stop_words_text);
        
 
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings);
 
    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const;
 
    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const;
 
    vector<Document> FindTopDocuments(const string& raw_query) const;
 
    int GetDocumentCount() const;
 
    int GetDocumentId(int index) const;
 
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const;
 
private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> document_ids_;
 
    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }
 
    static bool IsValidWord(const string& word);
 
    vector<string> SplitIntoWordsNoStop(const string& text) const;
 
    static int ComputeAverageRating(const vector<int>& ratings);
 
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
 
    QueryWord ParseQueryWord(const string& text) const;
 
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };
 
    Query ParseQuery(const string& text) const;

    double CalculateIDF(const string& word) const;
    
    
    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
};
 
 
template<typename StringContainer>
inline SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw invalid_argument("Some of stop words are invalid"s);
    }
}
 
template<typename DocumentPredicate>
inline vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const
{
    const auto query = ParseQuery(raw_query);
 
    auto matched_documents = FindAllDocuments(query, document_predicate);
 
    sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
        if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
            return lhs.rating > rhs.rating;
        }
        else {
            return lhs.relevance > rhs.relevance;
        }
        });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }
 
    return matched_documents;
}

template<typename DocumentPredicate>
inline vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const{
    map<int, double> document_to_relevance;
    for (const string& word : query.plus_words) {
        
        if (word_to_document_freqs_.count(word) != 0) {
           
        const double IDF = CalculateIDF(word);
        
        for (const auto& [document_id, TF] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += IDF * TF;
            }
        }
    }
}
 
    for (const string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) != 0) {
                for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                    document_to_relevance.erase(document_id);
                }
            }
        }
 
    vector<Document> matched_documents;
    for (const auto& [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
    }
    return matched_documents;
}