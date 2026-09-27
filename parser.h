#ifndef PARSER_H
#include <string>
#include <vector>

struct Order {
    int price;
    int quantity;
};

class Parser {
    public:
        Parser(std::string filename);
        void parseTrades();
    private:
        std::string filename;
        std::vector<Order> bids;
        std::vector<Order> asks;
};

#endif