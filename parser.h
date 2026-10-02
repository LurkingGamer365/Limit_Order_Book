#ifndef PARSER_H
#include <string>
#include <vector>
#include <map>

struct Order {
    int price;
    int quantity;
};

class Parser {
    public:
        Parser(std::string filename);
        void parseTrades();
    private:
        void handleOrder(Order o, char side);
        std::string filename;
        std::map<int, int> bids;
        std::map<int, int> asks;
};

#endif