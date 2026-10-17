#ifndef PARSER_H
#define PARSER_H

#include <string>
#include <vector>
#include "types.h"
#include "order_book.h"

enum class Action { Limit, Market, Cancel };

struct Command {
    Action action;
    Order order;
};

class Parser {
    public:
        Parser(std::string filename);
        bool parse(std::vector<Command>& commands);
    private:
        std::string filename;
};

#endif
