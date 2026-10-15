#ifndef PARSER_H
#define PARSER_H

#include <functional>
#include <list>
#include <map>
#include <string>
#include <unordered_map>

struct Order {
    int id;
    char side;      // 'B' or 'S'
    int price;
    int quantity;
};

// A price level is a FIFO queue of resting orders (front = oldest).
// std::list so an order can be removed from the middle in O(1) on cancel.
using Level = std::list<Order>;

// Where a resting order lives, so cancel can jump straight to it.
struct OrderLocation {
    char side;
    int price;
    Level::iterator it;
};

class Parser {
    public:
        Parser(std::string filename);
        void parseTrades();
        void printBook() const;
    private:
        void handleOrder(Order o, bool market);
        void cancelOrder(int id);
        void rest(const Order& o);

        std::string filename;
        // bids sorted high->low, asks low->high, so begin() is the best price on both sides
        std::map<int, Level, std::greater<int>> bids;
        std::map<int, Level> asks;
        // id -> location, for orders currently resting in the book only
        std::unordered_map<int, OrderLocation> orders;
};

#endif
