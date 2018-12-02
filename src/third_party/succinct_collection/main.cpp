#include <iostream>
#include "Succinct_Collection.h"

using namespace std;

int main() {
    string s = "{ \"cursor\" : { \"firstBatch\" : [ { \"_id\" : { \"$oid\" : \"5bfdd36dbd83160ff3a19d49\" }, \"y\" : 3, \"name\" : \"x\" }, { \"_id\" : { \"$oid\" : \"5bfdd3fff67d1860e21f143c\" } }, { \"_id\" : { \"$oid\" : \"5bfdd406f67d1860e21f143d\" }, \"name\" : \"x\" }, { \"_id\" : { \"$oid\" : \"5bfdd40ff67d1860e21f143e\" }, \"name\" : \"y\" }, { \"_id\" : { \"$oid\" : \"5bfdd410f67d1860e21f143f\" }, \"name\" : \"z\" } ], \"id\" : { \"$numberLong\" : \"0\" }, \"ns\" : \"db.x\" }, \"ok\" : 1 }";
    string c = "c";
    bool finished;
    Succinct_Collection sc(c, s, finished);

    sc.get_size();

    vector<pair<string, string>> query_vec;
    cout << sc.find_query(query_vec) << endl;

    cout << endl;

    query_vec.push_back({"name","\"x\""});
    cout << sc.find_query(query_vec) << endl;

    cout << endl;

    query_vec.push_back({"y",to_string(3)});
    cout << sc.find_query(query_vec) << endl;
}