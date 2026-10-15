#include "parser.h"

int main() {
    Parser p("dummyBook.txt");
    p.parseTrades();
    p.printBook();
    return 0;
}