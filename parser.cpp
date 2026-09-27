#include <iostream>
#include <fstream>
#include "parser.h"
using namespace std;

Parser::Parser(std::string filename){
    this->filename = filename;
}

void Parser::parseTrades(){
    ifstream file(this->filename);
    if(!file.is_open()){
        cout << "File didnt open";
    }
    string line;
    while(getline(file,line)){
        cout << line << endl;
    }
}
