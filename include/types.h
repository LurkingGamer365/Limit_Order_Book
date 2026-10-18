#ifndef TYPES_H
#define TYPES_H

#include <list>

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
    bool live = false;
};

#endif