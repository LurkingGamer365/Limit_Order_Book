#include <iostream>
#include <string>
#include <vector>
#include "order_book.h"
#include "parser.h"
using namespace std;

static void printTrades(const vector<Trade>& trades){
    for(const Trade& t : trades){
        cout << "TRADE " << t.quantity << " @ " << t.price
             << " (aggressor " << t.aggressorId << ", resting " << t.restingId << ")\n";
    }
}

int main(int argc, char* argv[]) {
    string filename = argc > 1 ? argv[1] : "data/dummyBook.txt";
    vector<Command> commands;
    if(!Parser(filename).parse(commands)) return 1;

    OrderBook book;
    vector<Trade> trades;
    for(const Command& c : commands){
        trades.clear();
        switch(c.action){
            case Action::Limit:
                book.addLimit(c.order, trades);
                printTrades(trades);
                break;
            case Action::Market: {
                int unfilled = book.addMarket(c.order, trades);
                printTrades(trades);
                if(unfilled > 0){
                    cout << "Market order " << c.order.id << ": " << unfilled << " unfilled, discarded\n";
                }
                break;
            }
            case Action::Cancel:
                if(book.cancel(c.order.id)) cout << "Cancelled " << c.order.id << "\n";
                else cout << "Cancel " << c.order.id << " rejected: not resting in book\n";
                break;
        }
    }
    book.print();
    return 0;
}
