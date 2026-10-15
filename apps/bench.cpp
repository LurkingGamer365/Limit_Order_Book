#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include "order_book.h"
#include "parser.h"
using namespace std;
using Clock = chrono::steady_clock;

static int64_t nanos(Clock::duration d){
    return chrono::duration_cast<chrono::nanoseconds>(d).count();
}

static void execute(OrderBook& book, const Command& c, vector<Trade>& trades, uint64_t& checksum){
    trades.clear();
    switch(c.action){
        case Action::Limit: book.addLimit(c.order, trades); break;
        case Action::Market: checksum += book.addMarket(c.order, trades); break;
        case Action::Cancel: checksum += book.cancel(c.order.id); break;
    }
    for(const Trade& t : trades) checksum += t.quantity;
}

static int64_t percentile(const vector<int64_t>& sorted, double p){
    size_t i = min(sorted.size() - 1, static_cast<size_t>(p * sorted.size()));
    return sorted[i];
}

static void report(const char* name, vector<int64_t>& samples){
    if(samples.empty()) return;
    sort(samples.begin(), samples.end());
    double mean = 0;
    for(int64_t s : samples) mean += s;
    mean /= samples.size();
    printf("%-8s %10zu %8.0f %8lld %8lld %8lld %8lld %10lld\n", name, samples.size(), mean,
           (long long)percentile(samples, 0.50), (long long)percentile(samples, 0.90),
           (long long)percentile(samples, 0.99), (long long)percentile(samples, 0.999),
           (long long)samples.back());
}

static int64_t timerResolution(){
    int64_t smallest = 0;
    for(int i = 0; i < 100000; ++i){
        auto t0 = Clock::now();
        auto t1 = Clock::now();
        while(t1 == t0) t1 = Clock::now();
        int64_t d = nanos(t1 - t0);
        if(smallest == 0 || d < smallest) smallest = d;
    }
    return smallest;
}

int main(int argc, char* argv[]){
    if(argc < 2){
        fprintf(stderr, "usage: %s <orders file>\n", argv[0]);
        return 1;
    }

    vector<Command> commands;
    auto parseStart = Clock::now();
    if(!Parser(argv[1]).parse(commands)) return 1;
    double parseMs = nanos(Clock::now() - parseStart) / 1e6;

    vector<Trade> trades;
    trades.reserve(1024);

    OrderBook throughputBook;
    uint64_t throughputChecksum = 0;
    auto runStart = Clock::now();
    for(const Command& c : commands) execute(throughputBook, c, trades, throughputChecksum);
    double runSeconds = nanos(Clock::now() - runStart) / 1e9;

    vector<int64_t> limitNs, marketNs, cancelNs;
    limitNs.reserve(commands.size());
    marketNs.reserve(commands.size());
    cancelNs.reserve(commands.size());

    OrderBook latencyBook;
    uint64_t latencyChecksum = 0;
    for(const Command& c : commands){
        auto t0 = Clock::now();
        execute(latencyBook, c, trades, latencyChecksum);
        auto t1 = Clock::now();
        int64_t ns = nanos(t1 - t0);
        if(c.action == Action::Limit) limitNs.push_back(ns);
        else if(c.action == Action::Market) marketNs.push_back(ns);
        else cancelNs.push_back(ns);
    }

    printf("orders:      %zu (parsed in %.0f ms, not timed)\n", commands.size(), parseMs);
    printf("throughput:  %.2f M orders/s (%.3f s total)\n", commands.size() / runSeconds / 1e6, runSeconds);
    printf("timer tick:  %lld ns (latencies below are rounded to this)\n", (long long)timerResolution());
    printf("checksum:    %llu%s\n\n", (unsigned long long)latencyChecksum,
           latencyChecksum == throughputChecksum ? "" : "  MISMATCH between runs");

    printf("%-8s %10s %8s %8s %8s %8s %8s %10s\n", "latency", "count", "mean", "p50", "p90", "p99", "p99.9", "max");
    printf("%-8s %10s %8s %8s %8s %8s %8s %10s\n", "(ns)", "", "", "", "", "", "", "");
    report("limit", limitNs);
    report("market", marketNs);
    report("cancel", cancelNs);

    return latencyChecksum == throughputChecksum ? 0 : 1;
}
