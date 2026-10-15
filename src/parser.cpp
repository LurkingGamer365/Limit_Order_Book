#include <iostream>
#include <fstream>
#include <sstream>
#include "parser.h"
using namespace std;

Parser::Parser(std::string filename){
    this->filename = filename;
}

void Parser::parseTrades(OrderBook& book){
    ifstream file(this->filename);
    if(!file.is_open()){
        cout << "File didnt open" << endl;
        return;
    }

    string line;
    while(getline(file,line)){
        stringstream ss(line);

        string action;
        ss >> action;
        if(action.empty() || action[0] == '#') continue;

        Order o;
        if(action == "A"){
            ss >> o.id >> o.side >> o.price >> o.quantity;
            if(ss) book.addLimit(o);
        } else if(action == "M"){
            ss >> o.id >> o.side >> o.quantity;
            o.price = 0;
            if(ss) book.addMarket(o);
        } else if(action == "C"){
            ss >> o.id;
            if(ss) book.cancel(o.id);
        } else {
            cout << "Unknown action: " << line << endl;
        }
    }
}
