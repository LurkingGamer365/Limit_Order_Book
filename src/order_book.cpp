#include <algorithm>
#include <iostream>
#include <utility>
#include "order_book.h"
using namespace std;

static void pushBack(Level& level, int id, vector<OrderNode>& nodes){
    OrderNode& node = nodes[id];
    node.prev = level.tail;
    node.next = -1;
    if(level.tail == -1) level.head = id;
    else nodes[level.tail].next = id;
    level.tail = id;
}

static void unlink(Level& level, int id, vector<OrderNode>& nodes){
    OrderNode& node = nodes[id];
    if(node.prev == -1) level.head = node.next;
    else nodes[node.prev].next = node.next;
    if(node.next == -1) level.tail = node.prev;
    else nodes[node.next].prev = node.prev;
    node.prev = -1;
    node.next = -1;
}

static void matchAgainst(Order& incoming, BookSide& book, vector<OrderNode>& nodes, bool market, vector<Trade>& trades){
    while(incoming.quantity > 0 && !book.empty()){
        if(!market && !book.crosses(incoming.price)) break;

        int levelPrice = book.bestPrice();
        Level& level = book.level(levelPrice);
        while(incoming.quantity > 0 && !level.empty()){
            int restingId = level.head;
            Order& resting = nodes[restingId].order;
            int traded = min(incoming.quantity, resting.quantity);
            incoming.quantity -= traded;
            resting.quantity -= traded;
            trades.push_back({incoming.id, restingId, levelPrice, traded});
            if(resting.quantity == 0){
                nodes[restingId].live = false;
                unlink(level, restingId, nodes);
            }
        }
        if(level.empty()) book.levelEmptied(levelPrice);
    }
}

OrderBook::OrderBook(int maxOrderId, int base, int size)
    : bids(true, base, size), asks(false, base, size), nodes(maxOrderId + 1) {}

void OrderBook::match(Order& o, bool market, vector<Trade>& trades){
    if(o.side == 'B') matchAgainst(o, this->asks, this->nodes, market, trades);
    else matchAgainst(o, this->bids, this->nodes, market, trades);
}

AddResult OrderBook::addLimit(Order o, vector<Trade>& trades){
    BookSide& side = (o.side == 'B') ? this->bids : this->asks;
    if(side.outRange(o.price)) return AddResult::PriceOutOfRange;
    if(o.id < 0 || o.id >= static_cast<int>(this->nodes.size())) return AddResult::IdOutOfRange;
    if(this->nodes[o.id].live) return AddResult::DuplicateId;
    this->match(o, false, trades);
    if(o.quantity > 0) this->rest(o);
    return AddResult::Accepted;
}

int OrderBook::addMarket(Order o, vector<Trade>& trades){
    this->match(o, true, trades);
    return o.quantity;
}

void OrderBook::rest(const Order& o){
    BookSide& bookSide = (o.side == 'B') ? this->bids : this->asks;
    Level& level = bookSide.level(o.price);
    if(level.empty()) bookSide.levelFilled(o.price);
    OrderNode& node = this->nodes[o.id];
    node.order = o;
    node.live = true;
    pushBack(level, o.id, this->nodes);
}

bool OrderBook::cancel(int id){
    if(id < 0 || id >= static_cast<int>(this->nodes.size())) return false;
    OrderNode& node = this->nodes[id];
    if(!node.live) return false;
    node.live = false;

    BookSide& bookSide = (node.order.side == 'B') ? this->bids : this->asks;
    Level& level = bookSide.level(node.order.price);
    unlink(level, id, this->nodes);
    if(level.empty()) bookSide.levelEmptied(node.order.price);
    return true;
}

static vector<pair<int, int>> levelsBestToWorst(const BookSide& side, const vector<OrderNode>& nodes, int step){
    vector<pair<int, int>> totals;
    int left = side.levelCount();
    for(int price = side.bestPrice(); left > 0; price += step){
        const Level& level = side.level(price);
        if(level.empty()) continue;
        int total = 0;
        for(int i = level.head; i != -1; i = nodes[i].next) total += nodes[i].order.quantity;
        totals.push_back({price, total});
        --left;
    }
    return totals;
}

void OrderBook::print() const {
    vector<pair<int, int>> askLevels = levelsBestToWorst(this->asks, this->nodes, 1);
    cout << "--- ASKS ---" << endl;
    for(auto it = askLevels.rbegin(); it != askLevels.rend(); ++it){
        cout << it->first << " x " << it->second << endl;
    }
    cout << "--- BIDS ---" << endl;
    for(const auto& [price, total] : levelsBestToWorst(this->bids, this->nodes, -1)){
        cout << price << " x " << total << endl;
    }
}
