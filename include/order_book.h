#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <functional>
#include <list>
#include <map>
#include <unordered_map>
#include <vector>

struct Order {
    int id;
    char side;
    int price;
    int quantity;
};

struct Trade {
    int aggressorId;
    int restingId;
    int price;
    int quantity;
};

using Level = std::list<Order>;

struct OrderLocation {
    char side;
    int price;
    Level::iterator it;
};

class OrderBook {
    public:
        void addLimit(Order o, std::vector<Trade>& trades);
        int addMarket(Order o, std::vector<Trade>& trades);
        bool cancel(int id);
        void print() const;
    private:
        void match(Order& o, bool market, std::vector<Trade>& trades);
        void rest(const Order& o);

        std::map<int, Level, std::greater<int>> bids;
        std::map<int, Level> asks;
        std::unordered_map<int, OrderLocation> orders;
};

#endif
