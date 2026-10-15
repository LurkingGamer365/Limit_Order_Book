#ifndef PARSER_H
#define PARSER_H

#include <string>
#include "order_book.h"

class Parser {
    public:
        Parser(std::string filename);
        void parseTrades(OrderBook& book);
    private:
        std::string filename;
};

#endif
