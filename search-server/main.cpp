#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
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
    int id;
    double relevance;
};

class SearchServer {

public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {

        ++document_count_;
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double tf = 1.0 / words.size();

        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += tf;           
        }
    }
 
    vector<Document> FindTopDocuments(const string& raw_query) const {

        auto query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {

            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:

    struct Query {
        set<string>minus_word;
        set<string>plus_word;
    };
    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };
    int document_count_ = 0;
    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
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
	
    QueryWord ParseQueryWord(string text) const {
          bool is_minus = false;
          if (text[0] == '-') {
              is_minus = true;
              text = text.substr(1);
    }
    return {text, is_minus, IsStopWord(text)};
}

    Query ParseQuery(const string &text) const {
          Query query_words;
			for (const string &word : SplitIntoWords(text)) {
				const QueryWord query_parse_word = ParseQueryWord(word);
					if (!query_parse_word.is_stop) {
					  if (query_parse_word.is_minus)
						 query_words.minus_word.insert(query_parse_word.data);
						else
						   query_words.plus_word.insert(query_parse_word.data);
        }
    }

		return query_words;
}


    double GetIDF(const string& word) const {
		
		   return static_cast<double> (log(document_count_ *1.0 
		                              / word_to_document_freqs_.at(word).size()));
	}
	
    vector<Document> FindAllDocuments(const Query& words) const {

        vector<Document> matched_documents;
        map<int, double> document_to_relevance;

        for (const auto& word : words.plus_word) {
          
            if (word_to_document_freqs_.count(word) != 0) {

                const double idf = GetIDF(word);

                for (const auto& [id, tf] : word_to_document_freqs_.at(word)) {
                    document_to_relevance[id] += tf * idf;
                }
            }
        }
        for (const auto& word : words.minus_word) {
           
            if (word_to_document_freqs_.count(word) != 0) {

                for (const auto& [id, value] : word_to_document_freqs_.at(word)) {
                    document_to_relevance.erase(value);
                }
            }
        }
        for (const auto& [id, relevance] : document_to_relevance) {
            matched_documents.push_back({ id, relevance });
        }
        return matched_documents;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();

    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
             << "relevance = "s << relevance << " }"s << endl;
    }
}