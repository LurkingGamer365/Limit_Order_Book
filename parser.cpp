#include <iostream>
#include <fstream>
#include <sstream>
#include "parser.h"
using namespace std;

Parser::Parser(std::string filename){
    this->filename = filename;
}

// Fill `incoming` against one side of the book: best price first, oldest order
// first within a price. Works for both sides because key_comp() is the book's
// sort order: comp(incoming.price, levelPrice) is true when the level is worse
// than our limit (ask above a buy limit, or bid below a sell limit).
template <typename Book>
static void match(Order& incoming, Book& book, unordered_map<int, OrderLocation>& orders, bool market){
    while(incoming.quantity > 0 && !book.empty()){
        auto levelIt = book.begin();
        int levelPrice = levelIt->first;
        if(!market && book.key_comp()(incoming.price, levelPrice)) break;

        Level& level = levelIt->second;
        while(incoming.quantity > 0 && !level.empty()){
            Order& resting = level.front();
            int traded = min(incoming.quantity, resting.quantity);
            incoming.quantity -= traded;
            resting.quantity -= traded;
            cout << "TRADE " << traded << " @ " << levelPrice
                 << " (aggressor " << incoming.id << ", resting " << resting.id << ")" << endl;
            if(resting.quantity == 0){
                orders.erase(resting.id);
                level.pop_front();
            }
        }
        if(level.empty()) book.erase(levelIt);
    }
}

void Parser::handleOrder(Order o, bool market){
    if(o.side == 'B') match(o, this->asks, this->orders, market);
    else match(o, this->bids, this->orders, market);

    if(o.quantity == 0) return;
    if(market){
        cout << "Market order " << o.id << ": " << o.quantity << " unfilled, discarded" << endl;
        return;
    }
    rest(o);
}

void Parser::rest(const Order& o){
    Level& level = (o.side == 'B') ? this->bids[o.price] : this->asks[o.price];
    level.push_back(o);
    this->orders[o.id] = {o.side, o.price, prev(level.end())};
}

void Parser::cancelOrder(int id){
    auto found = this->orders.find(id);
    if(found == this->orders.end()){
        cout << "Cancel " << id << " rejected: not resting in book" << endl;
        return;
    }
    OrderLocation loc = found->second;
    this->orders.erase(found);

    auto removeFrom = [&](auto& book){
        auto levelIt = book.find(loc.price);
        levelIt->second.erase(loc.it);
        if(levelIt->second.empty()) book.erase(levelIt);
    };
    if(loc.side == 'B') removeFrom(this->bids);
    else removeFrom(this->asks);
    cout << "Cancelled " << id << endl;
}

void Parser::printBook() const {
    cout << "--- ASKS ---" << endl;
    for(auto it = this->asks.rbegin(); it != this->asks.rend(); ++it){
        int total = 0;
        for(const Order& o : it->second) total += o.quantity;
        cout << it->first << " x " << total << endl;
    }
    cout << "--- BIDS ---" << endl;
    for(const auto& [price, level] : this->bids){
        int total = 0;
        for(const Order& o : level) total += o.quantity;
        cout << price << " x " << total << endl;
    }
}

void Parser::parseTrades(){
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
            if(ss) this->handleOrder(o, false);
        } else if(action == "M"){
            ss >> o.id >> o.side >> o.quantity;
            o.price = 0;  // unused: market orders take any price
            if(ss) this->handleOrder(o, true);
        } else if(action == "C"){
            ss >> o.id;
            if(ss) this->cancelOrder(o.id);
        } else {
            cout << "Unknown action: " << line << endl;
        }
    }
}
