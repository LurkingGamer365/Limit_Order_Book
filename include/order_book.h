#ifndef ORDER_BOOK_H
#define ORDER_BOOK_H

#include <vector>
#include "types.h"
#include "book_side.h"

enum class AddResult { Accepted, PriceOutOfRange, IdOutOfRange, DuplicateId };

class OrderBook {
    public:
        explicit OrderBook(int maxOrderId, int base = 0, int size = 65536);
        AddResult addLimit(Order o, std::vector<Trade>& trades);
        int addMarket(Order o, std::vector<Trade>& trades);
        bool cancel(int id);
        void print() const;
    private:
        void match(Order& o, bool market, std::vector<Trade>& trades);
        void rest(const Order& o);

        BookSide bids;
        BookSide asks;
        std::vector<OrderLocation> orders;
};

#endif
