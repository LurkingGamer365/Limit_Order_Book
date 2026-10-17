#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <functional>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>
#include "types.h"
#include "book_side.h"

class OrderBook {
    public:
        OrderBook(int base = 0, int size = 65536);
        void addLimit(Order o, std::vector<Trade>& trades);
        int addMarket(Order o, std::vector<Trade>& trades);
        bool cancel(int id);
        void print() const;
    private:
        void match(Order& o, bool market, std::vector<Trade>& trades);
        void rest(const Order& o);

        BookSide bids;
        BookSide asks;
        std::unordered_map<int, OrderLocation> orders;
};

#endif
