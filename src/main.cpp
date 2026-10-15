#include "order_book.h"
#include "parser.h"

int main(int argc, char* argv[]) {
    std::string filename = argc > 1 ? argv[1] : "data/dummyBook.txt";
    OrderBook book;
    Parser p(filename);
    p.parseTrades(book);
    book.print();
    return 0;
}
