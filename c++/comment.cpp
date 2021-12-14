#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cmath>
using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
		string s;
		getline(cin, s);
		return s;
}

int ReadLineWithNumber() {
		int result;
		cin >> result;
		ReadLine();
		return result;
}

vector<string> SplitIntoWords(const string& text) {
		vector<string> words;
		string word;
		for (const char c : text) {
				if (c == ' ') {
						words.push_back(word);
						word = "";
				} else {
						word += c;
				}
		}
		words.push_back(word);
		
		return words;
}

struct Document {
		int id;
		int relevance;
};

struct Query {
		vector<string> plus;
		vector<string> minus;
};

class SearchServer {
public:
		void SetStopWords(const string& text) {
				for (const string& word : SplitIntoWords(text)) {
						stop_words_.insert(word);
				}
		}

		void AddDocument(int document_id, const string& document) {
            Query parseQuery = ParseQuery(query);
			for (const string& word_document : SplitIntoWordsNoStop(document)) { 
                for (const string& word_query : parseQuery.plus) {
                    tf(word_query) = count(word_document.begin(), word_document.end(), word_query) / word_document.size();
                    ++document_count_(word_query);
                    word_to_document_freqs_[word_query].insert({document_id, tf});
                }
                //word_to_document_freqs_[word_document].insert({document_id, 0});
            }
        }
    
		vector<Document> FindTopDocuments(const string& query) const {
            Query parseQuery = ParseQuery(query);
            
				auto matched_documents = FindAllDocuments(parseQuery);
				
				sort(
						matched_documents.begin(),   
						matched_documents.end(), 
						[](const Document& lhs, const Document& rhs) {
								return lhs.relevance > rhs.relevance;
						}
				);
				if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
					matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
				}
				return matched_documents;
		}

private:
    //map<string, set<int>> word_to_documents_;
    map<string, map<int, double>> word_to_document_freqs_;
    set<string> stop_words_;
    int document_count_ = 0;
    vector<double> tf;
    //vector<int> document_count_;	
    
    vector<string> SplitIntoWordsNoStop(const string& text) const {
				vector<string> words;
				for (const string& word : SplitIntoWords(text)) {
						if (stop_words_.count(word) == 0) {
								words.push_back(word);
						}
				}
				return words;
		}

		vector<Document> FindAllDocuments(const Query& parseQuery) const {
				map<int, int> document_to_relevance;
            
				for (const string& word : parseQuery.plus) {
                    
						if (word_to_document_freqs_.count(word) == 0) {
								continue;
						}
						for (const auto [document_id, rang_tf_idf] : word_to_document_freqs_.at(word)) {
                            
							document_to_relevance[document_id] = rang_tf_idf;
						}
				}
				for (const string& word : parseQuery.minus) {
						if (word_to_document_freqs_.count(word) == 0) {
								continue;
						}
						for (const auto [document_id, rang_tf_idf] : word_to_document_freqs_.at(word)) {
								document_to_relevance.erase(document_id);
						}
				}		
				vector<Document> matched_documents;
				for (auto [document_id, relevance] : document_to_relevance) {
						matched_documents.push_back({document_id, relevance});
				}
			
				return matched_documents;
		}
        Query ParseQuery (const string& text) const {
            Query parseQuery;            
            for (const string& word : SplitIntoWordsNoStop(text)) {
                if (word[0] == '-') {
                    parseQuery.minus.push_back(word.substr(1));
                }
                else {
                    parseQuery.plus.push_back(word);
                }
            }
            
        return parseQuery;            
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
		for (auto [document_id, relevance] : search_server.FindTopDocuments(query)) {
				cout << "{ document_id = " << document_id << ", relevance = " << relevance << " }" << endl;
		}
}