#ifndef BOOK_SIDE_H
#define BOOK_SIDE_H

#include <vector>
#include "types.h"

class BookSide {
    public:
        BookSide(bool isBid, int base, int size);
        bool empty() const {return activeLevels == 0;}
        int bestPrice() const {return best + base; }
        int levelCount() const {return activeLevels;};
        bool outRange(int price) const {return price - base < 0 || price - base >= static_cast<int>(levels.size());}
        Level& level(int price) {return levels[price - base];}
        const Level& level(int price) const {return levels[price - base];}
        void levelEmptied(int price);
        void levelFilled(int price);
        bool crosses(int limitPrice) const;
    private:
        bool isBid;
        int base;
        int best;
        int activeLevels;
        std::vector<Level> levels;

};

#endif