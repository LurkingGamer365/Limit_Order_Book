#include "book_side.h"

BookSide::BookSide(bool isBid, int base, int size)
    : isBid(isBid), base(base), best(-1), activeLevels(0), levels(size) {}


void BookSide::levelEmptied(int price){
    --activeLevels;
    if(activeLevels == 0){
        best = -1;
        return;
    }
    if(price != bestPrice()){
        return;
    }
    while(levels[best].empty()){
        best += (isBid ? -1 : 1);
    }
}

void BookSide::levelFilled(int price){
    ++activeLevels;
    if(best == -1 || (isBid && price > bestPrice()) || (!isBid && price < bestPrice())){
        best = price - base;
    }
}

bool BookSide::crosses(int limitPrice) const {
    if(isBid){
        return bestPrice() >= limitPrice;
    } else {
        return bestPrice() <= limitPrice;
    }
}