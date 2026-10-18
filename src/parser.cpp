#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include "parser.h"
using namespace std;

Parser::Parser(std::string filename){
    this->filename = filename;
}

bool Parser::parse(vector<Command>& commands){
    ifstream file(this->filename);
    if(!file.is_open()){
        cerr << "File didnt open: " << this->filename << endl;
        return false;
    }

    string line;
    while(getline(file,line)){
        stringstream ss(line);

        string action;
        ss >> action;
        if(action.empty() || action[0] == '#') continue;

        Order o{};
        if(action == "A"){
            ss >> o.id >> o.side >> o.price >> o.quantity;
            if(ss) commands.push_back({Action::Limit, o});
        } else if(action == "M"){
            ss >> o.id >> o.side >> o.quantity;
            if(ss) commands.push_back({Action::Market, o});
        } else if(action == "C"){
            ss >> o.id;
            if(ss) commands.push_back({Action::Cancel, o});
        } else {
            cout << "Unknown action: " << line << endl;
        }
    }
    return true;
}

int maxOrderId(const vector<Command>& commands){
    int highest = 0;
    for(const Command& c : commands) highest = max(highest, c.order.id);
    return highest;
}
