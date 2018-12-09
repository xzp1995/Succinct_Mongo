#include "Succinct_Collection.h"
#include <iostream>
#include <fstream>

#include "json.hpp"
#include "succinct_shard.h"
#include <stdio.h>


using json = nlohmann::json;
using namespace std;

char Succinct_Collection::get_delimiter(const string &attribute) {
    if (attribute_delimiter_map.find(attribute)==attribute_delimiter_map.end()) {
        if (available_delimiters.empty()) {
            throw std::runtime_error("Not enough delimiters!");
        }
        char del = available_delimiters.top();
        available_delimiters.pop();
        attribute_delimiter_map[attribute] = del;
        delimiter_attribute_map[del] = attribute;
    }
    return attribute_delimiter_map[attribute];
}


json string_to_json(const string& delimiter_string,unordered_map<char, string>& delimiter_attribute_map) {
    //get positions of delimiters in the string
    vector<unsigned int> delimiter_pos;
    for (unsigned int i=0; i<delimiter_string.size(); i++) {
        if (delimiter_attribute_map.find(delimiter_string[i]) != delimiter_attribute_map.end()) {
            delimiter_pos.push_back(i);
        }
    }
    //convert document string to document json
    json doc_json;
    //process id field
    string id_str = delimiter_string.substr(0, delimiter_pos[0]);
    doc_json["_id"]["$oid"] = id_str;
    //process subsequent fields
    for (int i=1; i<delimiter_pos.size(); i++) {
        string attribute_str = delimiter_attribute_map[delimiter_string.at(delimiter_pos[i])];
        string value_str = delimiter_string.substr(delimiter_pos[i-1]+2,delimiter_pos[i]-delimiter_pos[i-1]-2);
        if (value_str[0] == '"') {
            //string type value
            doc_json[attribute_str] = value_str.substr(1, value_str.size()-2);
        }
        else {
            //number type value, convert to double
            doc_json[attribute_str] = stod(value_str);
        }
    }
    return doc_json;
}


int Succinct_Collection::insert_string(string& json_string) {
    ofstream succinct_file;
    succinct_file.open(plain_filename);
    json j = json::parse(json_string);
    json batch_json = j.begin().value().begin().value();
    int insert_count = 0;
    for (json::iterator batch_it=batch_json.begin(); batch_it!=batch_json.end(); batch_it++) {
        insert_count++;
        //process id field
        succinct_file << "\n";
        string id_attr = batch_it.value().begin().value().begin().key();
        string id_value = batch_it.value().begin().value().begin().value();
        succinct_file << id_value << get_delimiter(id_attr) << " ";
        //process doc fields
        json doc_json = batch_it.value();
        json::iterator doc_it = doc_json.begin();
        doc_it++;
        for (; doc_it!=doc_json.end(); doc_it++) {
            const string &attribute = doc_it.key();
            const auto value = doc_it.value();
            char delimiter = get_delimiter(attribute);
            succinct_file << value << delimiter << " ";
        }
    }
    curr_doc_count += insert_count;
    succinct_file.close();
    if (curr_doc_count >= total_doc_count) {
        succinct_fd = new SuccinctShard(0, plain_filename);
        succinct_file.close();
        remove(plain_filename.c_str());
        return 1;
    }
    return 0;
}


Succinct_Collection::Succinct_Collection(string& collection_name, string& json_string, int doc_count) {
    //assign plain file name
    srand (time(NULL));
    string file_location = collection_name+"_"+to_string(rand()%10000);
    plain_filename = file_location+".txt";
    succinct_path = file_location+".succinct";
    ofstream succinct_file;
    succinct_file.open(plain_filename);

    //assign delimiter list
    for (int i=0; i<128; i++) available_delimiters.push((char)i);

    //parse json
    total_doc_count = doc_count;
    curr_doc_count = 0;
    insert_string(json_string);
}


Succinct_Collection::~Succinct_Collection() {
    if (succinct_fd) {
        auto succinct_ptr = (SuccinctShard*)succinct_fd;
        delete succinct_ptr;
    }
}


set<int64_t> Succinct_Collection::find_key(vector<pair<string, string>>& attr_val_vec) {
    set<int64_t> result;
    if (!succinct_fd) {
        cout << "Succinct build unfinished!" << endl;
        return result;
    }
    set<int64_t> query_result, intersect;
    //check if all attributes are valid
    for (pair<string, string>& attr_val : attr_val_vec) {
        cout << "find pair: " << attr_val.first << " " << attr_val.second << endl;
        if (attribute_delimiter_map.find(attr_val.first)==attribute_delimiter_map.end()) {
            return result;
        }
    }
    auto succinct_ptr = (SuccinctShard*)succinct_fd;
    size_t num_keys = succinct_ptr->GetNumKeys()-1; //-1 since the first row is a "\n"
    for (int i=1; i<=num_keys; i++) result.insert(i);
    for (pair<string, string> attr_val : attr_val_vec) {
        string query_string = " " + attr_val.second + attribute_delimiter_map[attr_val.first]+" ";
        succinct_ptr->Search(query_result, query_string);
        set_intersection(result.begin(),result.end(),query_result.begin(),query_result.end(),
                         inserter(intersect,intersect.begin()));
        swap(result, intersect);
        query_result.clear();
        intersect.clear();
    }
    return result;
}

vector<string> Succinct_Collection::find_query(vector<pair<string, string>>& attr_val_vec, int batch_size) {
    if (!succinct_fd) {
        cout << "Succinct build unfinished!" << endl;
        vector<string> res;
        return res;
    }
    find_results = find_key(attr_val_vec);
    find_cursor = find_results.begin();
    find_has_next = !find_results.empty();
    return find_next(batch_size);
}


vector<string> Succinct_Collection::find_next(int batch_size) {
    vector<string> result;
    if (!succinct_fd) {
        cout << "Succinct build unfinished!" << endl;
        return result;
    }
    if (!find_has_next) return result;
    auto succinct_ptr = (SuccinctShard*)succinct_fd;
    for (; find_cursor != find_results.end() && result.size() < batch_size; find_cursor++) {
        string doc_string;
        succinct_ptr->Get(doc_string, *find_cursor);
        json doc_json = string_to_json(doc_string, delimiter_attribute_map);
        result.push_back(doc_json.dump());
    }
    find_has_next = (find_cursor!=find_results.end());
    return result;
}


pair<size_t, size_t> Succinct_Collection::get_size() {
    if (!succinct_fd) {
        cout << "Succinct build unfinished!" << endl;
        return {0,0};
    }
    auto succinct_ptr = (SuccinctShard*)succinct_fd;
    return {succinct_ptr->StorageSize(), succinct_ptr->GetOriginalSize()};
}


void Succinct_Collection::serialize_succinct(){
    if (!succinct_fd) {
        cout << "Succinct build unfinished!" << endl;
    }
    auto succinct_ptr = (SuccinctShard*)succinct_fd;
    succinct_ptr->Serialize(succinct_path);
}