#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <set>
#include <string>
#include <optional>
#include <numeric>
#include <map>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

struct Document {
    Document() = default;

    Document(int id_, double relevance_, int rating_)
        : id(id_), relevance(relevance_), rating(rating_)
    {}

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

template <typename StringContainer>

set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

class SearchServer {
public:
    template <typename StringContainer>
     explicit SearchServer(const StringContainer& stop_words) {
        if (all_of(stop_words.begin(), stop_words.end(), IsValidWord)) {
            stop_words_ = MakeUniqueNonEmptyStrings(stop_words);
        }
        else {
            throw invalid_argument("слово содержит специальный символ"s);
        }
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))
    {
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0) {
            throw invalid_argument("документ с отрицательным id"s);
        }
        if (documents_.count(document_id)) {
            throw invalid_argument("документ c id ранее добавленного документа"s);
        }
        if (!IsValidWord(document)) {
            throw invalid_argument("наличие недопустимых символов"s);
        }

        const vector<string> words = SplitIntoWordsNoStop(document);
        const double tf = 1.0 / words.size();
        for (auto& word : words) {
            word_to_document_freqs_[word][document_id] += tf;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });

        documents_index_.push_back(document_id);

    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                const double EPSILON = 1e-6;
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

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query,
            [&status](int document_id, DocumentStatus new_status, int rating) {
                return new_status == status;
            });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

     int GetDocumentId(int index) const {
       return documents_index_.at(index);
     }
   

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        Query query = ParseQuery(raw_query);

        vector<string> matched_words;
        for (const string& word : query.plus_word) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_word) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }

        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct Query {
        set<string> plus_word;
        set<string> minus_word;
    };

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    set<string> stop_words_;
    vector<int> documents_index_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string& word) {
        
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = accumulate(ratings.begin(),ratings.end(),0);
       
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        QueryWord result;
         bool is_minus = false; 
        
        if (!IsValidWord(text)) { 
            throw invalid_argument(" invalid_argument(слово содержит не допустимые символы"); 
        } 
       
        if (text[0] == '-') { 
            is_minus = true; 
            text = text.substr(1); 
        } 
         
        
        if (text.empty() || text[0] == '-') { 
            throw invalid_argument("наличие более чем одного минуса перед словами или отсутствие слова после минуса"s);
        } 
         
        return { text, is_minus, IsStopWord(text) }; 
    } 
        
    Query ParseQuery(const string& text) const {
        Query query_words;

        for (const string& word : SplitIntoWords(text)) {
            QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query_words.minus_word.insert(query_word.data);
                }
                else {
                    query_words.plus_word.insert(query_word.data);
                }
            }
        }
        return query_words;
    }

    double GetIDF(const string& word) const {

        return static_cast<double> ( log (GetDocumentCount() * 1.0
            / word_to_document_freqs_.at(word).size()));
    }

    template<typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query_words, DocumentPredicate predicate) const {

        map<int, double> document_to_relevance;
        for (const string& word : query_words.plus_word) {
            
            if (word_to_document_freqs_.count(word) != 0) {

                const double IDF = GetIDF(word);

                for (const auto& [document_id, TF] : word_to_document_freqs_.at(word)) {

                    if (predicate(document_id, documents_.at(document_id).status, documents_.at(document_id).rating)) {
                        
                        document_to_relevance[document_id] += IDF * TF;
                    }
                }
            }
        }
        for (const string& word : query_words.minus_word) {
           
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
};

// ------------ Пример использования ----------------

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s << endl;
}
int main() {

    setlocale(LC_ALL, "RU"); 

    try
    {
        SearchServer search_server("и в на"s);
        (void)search_server.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
        (void)search_server.AddDocument(1, "золотая рыбка"s, DocumentStatus::ACTUAL, { 1, 2 });
        (void)search_server.AddDocument(-3, "изумительный язык С"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
        (void)search_server.AddDocument(4, "большой пёс овча\x10рка "s, DocumentStatus::ACTUAL, { 1, 3, 2, 4 });
        (void)search_server.AddDocument(5, "пушистый кот пушистый хвостcc"s, DocumentStatus::ACTUAL, { 7, 2, 7 });

        vector<Document> documents = search_server.FindTopDocuments("пушистый кот"s);

        for (const Document& document : documents) {
            PrintDocument(document);
        }
    }

    catch (const invalid_argument& e) {
        cout << "Error: "s << e.what() << endl;
    }
}