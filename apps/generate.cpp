#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>
using namespace std;

const double cancelRatio = 0.45;
const double marketRatio = 0.03;
const double aggressiveRatio = 0.05;
const double depthFalloff = 0.15;
const int startingMid = 10000;
const int driftEvery = 500;

int main(int argc, char* argv[]){
    if(argc < 3){
        cerr << "usage: " << argv[0] << " <orders> <output> [seed]" << endl;
        return 1;
    }
    long count = stol(argv[1]);
    string path = argv[2];
    uint64_t seed = argc > 3 ? stoull(argv[3]) : 42;

    ofstream out(path);
    if(!out){
        cerr << "cannot open " << path << endl;
        return 1;
    }

    mt19937_64 rng(seed);
    uniform_real_distribution<double> unit(0.0, 1.0);
    geometric_distribution<int> depth(depthFalloff);
    uniform_int_distribution<int> lots(1, 10);
    uniform_int_distribution<int> aggression(1, 4);
    uniform_int_distribution<int> drift(-1, 1);

    int mid = startingMid;
    int nextId = 1;
    vector<int> live;

    for(long i = 0; i < count; ++i){
        if(i % driftEvery == 0) mid += drift(rng);

        double roll = unit(rng);
        char side = unit(rng) < 0.5 ? 'B' : 'S';

        if(roll < cancelRatio && !live.empty()){
            uniform_int_distribution<size_t> pick(0, live.size() - 1);
            size_t k = pick(rng);
            out << "C " << live[k] << '\n';
            live[k] = live.back();
            live.pop_back();
        } else if(roll < cancelRatio + marketRatio){
            out << "M " << nextId++ << ' ' << side << ' ' << lots(rng) * 10 << '\n';
        } else {
            int offset = unit(rng) < aggressiveRatio ? -aggression(rng) : depth(rng);
            int price = side == 'B' ? mid - 1 - offset : mid + 1 + offset;
            out << "A " << nextId << ' ' << side << ' ' << price << ' ' << lots(rng) * 10 << '\n';
            live.push_back(nextId++);
        }
    }
    return 0;
}
