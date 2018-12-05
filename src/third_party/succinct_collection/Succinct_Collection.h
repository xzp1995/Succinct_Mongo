#ifndef S_SUCCINCT_COLLECTION_H
#define S_SUCCINCT_COLLECTION_H

#include <unordered_map>
#include <vector>
#include <string>
#include <stack>
#include <set>


using namespace std;


class Succinct_Collection {
private:
    stack<char> available_delimiters;   //delimiters available when parsing json
    unordered_map<string, char> attribute_delimiter_map;    //map from json attribute to delimiter
    unordered_map<char, string> delimiter_attribute_map;    //map from delimiter to json attribute
    string succinct_path = "";
    string plain_filename;
    void *succinct_fd;  //void* to succinct file descriptor, need explicit conversion to SuccinctShard* when use
    set<int64_t> find_results;
    bool find_has_next;
    set<int64_t>::iterator find_cursor;
    int total_doc_count;
    int curr_doc_count;

    set<int64_t> find_key(vector<pair<string, string>>& attr_val_vec);    //find keys satisfying attr-val pairs
    char get_delimiter(const string& attribute);

public:
    Succinct_Collection(string& collection_name, string& json_string, int doc_count);  //f_c=1 if finished
    ~Succinct_Collection();
    void insert_string(string& json_string);  //string containing json and batch size, return 1 on end of doc
    pair<size_t, size_t> get_size();    //return {succinct size, original size}
    void serialize_succinct();  //serialize succinct to succinct path directory
    vector<string> find_query(vector<pair<string, string>>& attr_val_vec, int batch_size);  //find exact match, size<=batch_size
    vector<string> find_next(int batch_size);   //get next batch_size query results
};

#endif //S_SUCCINCT_COLLECTION_H
