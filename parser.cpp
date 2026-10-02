#include <iostream>
#include <fstream>
#include <sstream>
#include "parser.h"
using namespace std;

Parser::Parser(std::string filename){
    this->filename = filename;
}

void Parser::handleOrder(Order o, char side){
    if(side == 'B'){
        if(this->asks.empty() || o.price < this->asks.begin()->first){
            this->bids[o.price] += o.quantity;
            return;
        }
        int topPrice = this->asks.begin()->first;
        while(topPrice <= o.price && o.quantity > 0){
            if(o.quantity >= this->asks[topPrice]){
                o.quantity -= this->asks[topPrice];
                this->asks.erase(topPrice);
            } else {
                this->asks[topPrice] -= o.quantity;
                o.quantity = 0;
            }
            topPrice = this->asks.begin()->first;
        }
        this->bids[o.price] += o.quantity;
    } else {
        if(this->bids.empty() || o.price > this->bids.rbegin()->first){
            this->asks[o.price] += o.quantity;
            return;
        }
        int topPrice = this->bids.rbegin()->first;
        while(topPrice >= o.price && o.quantity > 0){
            if(o.quantity >= this->bids[topPrice]){
                o.quantity -= this->bids[topPrice];
                this->bids.erase(topPrice);
            } else {
                this->bids[topPrice] -= o.quantity;
                o.quantity = 0;
            }
            topPrice = this->bids.rbegin()->first;
        }
        this->asks[o.price] += o.quantity;
    }
}

void Parser::parseTrades(){
    ifstream file(this->filename);
    if(!file.is_open()){
        cout << "File didnt open";
    }

    string line;

    while(getline(file,line)){
        stringstream ss(line);

        string action;
        ss >> action;
        if (action[0] == '#' || action.empty()) continue;

        int id, price, quantity;
        char side;

        ss >> id >> side >> price >> quantity;

        Order o;
        o.price = price;
        o.quantity = quantity;
        this->handleOrder(o,side);
        // cout << this->bids.size() << endl;
        // cout << line << endl;
    }
}
