#ifndef TYPES_H
#define TYPES_H

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

struct Level {
    int head = -1;
    int tail = -1;
    bool empty() const { return head == -1; }
};

struct OrderNode {
    Order order;
    int prev = -1;
    int next = -1;
    bool live = false;
};

#endif
