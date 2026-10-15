#include <algorithm>
#include <iostream>
#include <iterator>
#include "order_book.h"
using namespace std;

template <typename Book>
static void matchAgainst(Order& incoming, Book& book, unordered_map<int, OrderLocation>& orders, bool market, vector<Trade>& trades){
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
            trades.push_back({incoming.id, resting.id, levelPrice, traded});
            if(resting.quantity == 0){
                orders.erase(resting.id);
                level.pop_front();
            }
        }
        if(level.empty()) book.erase(levelIt);
    }
}

void OrderBook::match(Order& o, bool market, vector<Trade>& trades){
    if(o.side == 'B') matchAgainst(o, this->asks, this->orders, market, trades);
    else matchAgainst(o, this->bids, this->orders, market, trades);
}

void OrderBook::addLimit(Order o, vector<Trade>& trades){
    this->match(o, false, trades);
    if(o.quantity > 0) this->rest(o);
}

int OrderBook::addMarket(Order o, vector<Trade>& trades){
    this->match(o, true, trades);
    return o.quantity;
}

void OrderBook::rest(const Order& o){
    Level& level = (o.side == 'B') ? this->bids[o.price] : this->asks[o.price];
    level.push_back(o);
    this->orders[o.id] = {o.side, o.price, prev(level.end())};
}

bool OrderBook::cancel(int id){
    auto found = this->orders.find(id);
    if(found == this->orders.end()) return false;
    OrderLocation loc = found->second;
    this->orders.erase(found);

    auto removeFrom = [&](auto& book){
        auto levelIt = book.find(loc.price);
        levelIt->second.erase(loc.it);
        if(levelIt->second.empty()) book.erase(levelIt);
    };
    if(loc.side == 'B') removeFrom(this->bids);
    else removeFrom(this->asks);
    return true;
}

void OrderBook::print() const {
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
