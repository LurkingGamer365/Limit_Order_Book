#include <algorithm>
#include <iostream>
#include <iterator>
#include <utility>
#include "order_book.h"
using namespace std;

static void matchAgainst(Order& incoming, BookSide& book, unordered_map<int, OrderLocation>& orders, bool market, vector<Trade>& trades){
    while(incoming.quantity > 0 && !book.empty()){
        if(!market && !book.crosses(incoming.price)) break;

        int levelPrice = book.bestPrice();
        Level& level = book.level(levelPrice);
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
        if(level.empty()) book.levelEmptied(levelPrice);
    }
}

OrderBook::OrderBook(int base, int size) 
    : bids(true, base, size), asks(false, base, size) {}

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
    BookSide& bookSide = (o.side == 'B') ? this->bids : this->asks;
    Level& level = bookSide.level(o.price);
    if(level.empty()) bookSide.levelFilled(o.price);
    level.push_back(o);
    this->orders[o.id] = {o.side, o.price, prev(level.end())};
}

bool OrderBook::cancel(int id){
    auto found = this->orders.find(id);
    if(found == this->orders.end()) return false;
    OrderLocation loc = found->second;
    this->orders.erase(found);

    BookSide& bookSide = (loc.side == 'B') ? this->bids : this->asks;
    Level& level = bookSide.level(loc.price);
    level.erase(loc.it);
    if(level.empty()) bookSide.levelEmptied(loc.price);
    return true;
}

static vector<pair<int, int>> levelsBestToWorst(const BookSide& side, int step){
    vector<pair<int, int>> totals;
    int left = side.levelCount();
    for(int price = side.bestPrice(); left > 0; price += step){
        const Level& level = side.level(price);
        if(level.empty()) continue;
        int total = 0;
        for(const Order& o : level) total += o.quantity;
        totals.push_back({price, total});
        --left;
    }
    return totals;
}

void OrderBook::print() const {
    vector<pair<int, int>> askLevels = levelsBestToWorst(this->asks, 1);
    cout << "--- ASKS ---" << endl;
    for(auto it = askLevels.rbegin(); it != askLevels.rend(); ++it){
        cout << it->first << " x " << it->second << endl;
    }
    cout << "--- BIDS ---" << endl;
    for(const auto& [price, total] : levelsBestToWorst(this->bids, -1)){
        cout << price << " x " << total << endl;
    }
}
