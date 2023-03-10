# Поисковый сервер
В рамках проекта реализована поисковая система, которая индексирует документы для последующего поиска по ключевым словам. При выполнении поискового запроса система выдает топ документов, наиболее релевантных поисковому запросу. Релевантность рассчитывается путем подсчета суммы значений TF-IDF - меры, которая вычисляется для каждого ключевого слова из поискового запроса в контексте слов текущего документа. TF-IDF ключевого слова прямо пропорциональна частоте употребления этого слова в документе и обратно пропорциональна частоте употребления слова во всех документах поисковой системы.

Реализована поддержка стоп-слов (слова, не учитываемые при поиске) и минус-слов (документ, содержащий минус-слово, не должен включаться в результаты поиска). 

Дополнительно реализован многопоточный поиск документов. В файле "main.cpp" содержится бенчмарк, в рамках которого генерируется поисковый индекс из 100 тыс. документов, и затем выполняются 100 поисковых запросов. Бенчмарк выполняется для однопоточной и многопоточной версии алгоритма, что позволяет оценить выигрыш в производительности. Для реализации многопоточного алгоритма поиска была разработана потокобезопасная версия std::unordered_map для случая с целочисленными ключами.
## Пример использования
Создаем экземпляр поискового сервера, при создании передаем строку стоп-слов: 
```C++
SearchServer search_server("and with");
```
Сохраняем документы в поисковый индекс:
```C++
int id = 0;
for (const std::string& text : {
    "white cat and yellow hat",
    "curly cat curly tail",
    "nasty dog with big eyes",
    "nasty pigeon john"
  }) {
  search_server.AddDocument(++id, text);
}
```
Выполняем запрос:
```C++
const size_t max_documents = 5;
std::cout << "Results for query = \"curly nasty cat\":" << std::endl;
for (const auto& result : search_server.Find("curly nasty cat", max_documents)) {
    std::cout << "{ "
        << "document_id = " << result.id << ", "
        << "relevance = " << result.relevance << " }" << std::endl;
}
```
Получаем вывод:
```
Results for query = "curly nasty cat":
{ document_id = 2, relevance = 0.866434 }
{ document_id = 4, relevance = 0.231049 }
{ document_id = 3, relevance = 0.173287 }
{ document_id = 1, relevance = 0.173287 }
```
## Используемые технологии
- С++17
- STL
## Описание исходных файлов
- search_server.h, search_server.cpp: содержат класс SearchServer, реализующий функционал поисковой системы.
- concurrent_map.h: содержит вспомогательный класс ConcurrentMap, который реализует потокобезопасную версию std::unordered_map для ключей, являющихся целыми числами.
- string_processing.h, string_processing: содержат вспомогательные функции для работы со строками.
- log_duration.h: содержит вспомогательный класс LogDuration и макросы на его основе, используемые для замеров времени выполнения операций. 
- main.cpp: содержит бенчмарк, который рассчитывает суммарную релевантность документов, найденных для 100 поисковых запросов по базе из 100 тыс. документов, сгенерированных случайным образом. Бенчмарк выполняется для однопоточной и многопоточной версии поиска. Пример вывода:
```
212.8
seq: 42257 ms
212.8
par: 17999 ms
```
## Планы по доработке
- Вынести бенчмарк в отдельный класс.
- Структурировать код с помощью пространств имен.
- Добавить сборку с помощью CMake.
