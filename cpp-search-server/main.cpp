#include "document.h"
#include "document.cpp"

#include "search_server.h"
#include "search_server.cpp"

#include "string_processing.h"
#include "string_processing.cpp"

#include "paginator.h"

#include "read_input_functions.h"
#include "read_input_functions.cpp"

#include "request_queue.h"
#include "request_queue.cpp"

#include "remove_duplicates.h"
#include "remove_duplicates.cpp"

#include "test_example_functions.h"
#include "test_example_functions.cpp"

#include "log_duration.h"

void PrintDocument(const Document &document)
{
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string> &words, DocumentStatus status)
{
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string &word : words)
    {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer &search_server, int document_id, const string &document, DocumentStatus status,
                 const vector<int> &ratings)
{
    try
    {
        search_server.AddDocument(document_id, document, status, ratings);
    }
    catch (const exception &e)
    {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer &search_server, const string &raw_query)
{
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try
    {
        for (const Document &document : search_server.FindTopDocuments(raw_query))
        {
            PrintDocument(document);
        }
    }
    catch (const exception &e)
    {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

int main() {
    TestSearchServer();
    SearchServer search_server("and with"s);

    AddDocument(search_server, 1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // дубликат документа 2, будет удалён
    AddDocument(search_server, 3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // отличие только в стоп-словах, считаем дубликатом
    AddDocument(search_server, 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, считаем дубликатом документа 1
    AddDocument(search_server, 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // добавились новые слова, дубликатом не является
    AddDocument(search_server, 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

    // множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
    AddDocument(search_server, 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

    // есть не все слова, не является дубликатом
    AddDocument(search_server, 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

    // слова из разных документов, не является дубликатом
    AddDocument(search_server, 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});
    
    cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << endl;
    RemoveDuplicates(search_server);
    cout << "After duplicates removed: "s << search_server.GetDocumentCount() << endl;
}