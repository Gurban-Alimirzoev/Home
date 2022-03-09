#include "search_server.h"

using namespace std;

SearchServer::SearchServer()
{
}

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer &stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord))
    {
        throw invalid_argument("Some of stop words are invalid"s);
    }
}

SearchServer::SearchServer(const string &stop_words_text)
    : SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const string &document, DocumentStatus status, const vector<int> &ratings)
{
    if ((document_id < 0) || (documents_.count(document_id) > 0))
    {
        throw invalid_argument("Invalid document_id"s);
    }
    const auto words = SplitIntoWordsNoStop(document);

    const double inv_word_count = 1.0 / words.size();
    for (const string &word : words)
    {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.insert(document_id);
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const string &raw_query, DocumentPredicate document_predicate) const
{
    const auto query = ParseQuery(raw_query);

    auto matched_documents = FindAllDocuments(query, document_predicate);

    sort(matched_documents.begin(), matched_documents.end(), [](const Document &lhs, const Document &rhs)
         {
                if (std::abs(lhs.relevance - rhs.relevance) < precision) {
                    return lhs.rating > rhs.rating;
                } 
                else {
                    return lhs.relevance > rhs.relevance;
                } });
    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
    {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;
}

vector<Document> SearchServer::FindTopDocuments(const string &raw_query, DocumentStatus status) const
{
    return SearchServer::FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating)
                                          { return document_status == status; });
}

vector<Document> SearchServer::FindTopDocuments(const string &raw_query) const
{
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const
{
    return documents_.size();
}

std::set<int>::const_iterator SearchServer::begin() const
{
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const
{
    return document_ids_.end();
}

const std::map<std::string, double> &SearchServer::GetWordFrequencies(int document_id) const
{
    static const std::map<std::string, double> result;
    if (document_to_word_freqs_.count(document_id) == 0)
    {
        return result;
    }

    return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id)
{
    if (!binary_search(document_ids_.begin(), document_ids_.end(), document_id))
    {
        return;
    }
    auto word_freq = document_to_word_freqs_.at(document_id);
    std::set<std::string> words;
    for (auto word_and_its_freq : word_freq)
    {
        words.insert(word_and_its_freq.first);
    }
    for (auto word : words)
    {
        word_to_document_freqs_.at(word).erase(document_id);
    }
    documents_.erase(document_id);
    document_to_word_freqs_.erase(document_id);
    document_ids_.erase(document_id);
}

tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string &raw_query, int document_id) const
{
    const auto query = ParseQuery(raw_query);

    vector<string> matched_words;
    for (const string &word : query.plus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.push_back(word);
        }
    }
    for (const string &word : query.minus_words)
    {
        if (word_to_document_freqs_.count(word) == 0)
        {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id))
        {
            matched_words.clear();
            break;
        }
    }
    return {matched_words, documents_.at(document_id).status};
}

bool SearchServer::IsStopWord(const string &word) const
{
    return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(const string &word)
{
    return none_of(word.begin(), word.end(), [](char c)
                   { return c >= '\0' && c < ' '; });
}

vector<string> SearchServer::SplitIntoWordsNoStop(const string &text) const
{
    vector<string> words;
    for (const string &word : SplitIntoWords(text))
    {
        if (!IsValidWord(word))
        {
            throw invalid_argument("Word "s + word + " is invalid"s);
        }
        if (!IsStopWord(word))
        {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int> &ratings)
{
    if (ratings.empty())
    {
        return 0;
    }
    int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string &text) const
{
    if (text.empty())
    {
        throw invalid_argument("Query word is empty"s);
    }
    string word = text;
    bool is_minus = false;
    if (word[0] == '-')
    {
        is_minus = true;
        word = word.substr(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word))
    {
        throw invalid_argument("Query word "s + text + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const string &text) const
{
    SearchServer::Query result;
    for (const string &word : SplitIntoWords(text))
    {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop)
        {
            if (query_word.is_minus)
            {
                result.minus_words.insert(query_word.data);
            }
            else
            {
                result.plus_words.insert(query_word.data);
            }
        }
    }
    return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string &word) const
{
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

template <typename DocumentPredicate>
    vector<Document> SearchServer::FindAllDocuments(const Query &query, DocumentPredicate document_predicate) const
    {
        map<int, double> document_to_relevance;
        for (const string &word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
            {
                const auto &document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating))
                {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string &word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
            {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word))
            {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance)
        {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }