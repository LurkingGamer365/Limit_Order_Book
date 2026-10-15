# Limit Order Book

[![CI](https://github.com/LurkingGamer365/Limit_Order_Book/actions/workflows/ci.yml/badge.svg)](https://github.com/LurkingGamer365/Limit_Order_Book/actions/workflows/ci.yml)

A limit order book and matching engine in C++17. It matches orders by price-time priority and supports limit orders, market orders and cancels. It also comes with a reproducible order generator, a latency benchmark, and tests that run in CI under sanitizers on Linux and macOS.

## Features

- **Price-time priority matching:** the best price fills first, and within a price the oldest order fills first.
- **Limit orders:** cross the spread as far as their limit price allows, then rest in the book.
- **Market orders:** fill at any price. Whatever can't be filled is discarded.
- **Cancels:** an order can be cancelled by id in O(1). Cancelling an order that has already been filled, or never existed, is rejected.
- **Partial fills:** an order can trade against several resting orders and price levels.

## Design

| Component | Data structure | Why |
|---|---|---|
| Price levels | `std::map<int, Level>` | Sorted by price. Bids use `std::greater`, so `begin()` is the best price on both sides. |
| Orders at one price | `std::list<Order>` | A FIFO queue for time priority. An order can be removed from the middle in O(1). |
| Order lookup | `std::unordered_map<int, OrderLocation>` | Maps an order id to its side, price and list iterator, so a cancel goes straight to the order. |

| Operation | Cost |
|---|---|
| Add an order that doesn't trade | O(log P) |
| Cancel | O(1) to find, O(log P) if its price level becomes empty |
| Match | O(log P) per price level consumed, plus O(1) per order filled |

P is the number of price levels. `OrderBook` doesn't print anything: it returns `Trade`s to the caller, so the matching engine can be timed on its own.

## Project layout

```
include/   order_book.h, parser.h     public headers
src/       order_book.cpp, parser.cpp the order book and input parser
apps/      main.cpp                   lob:   replays an order file and prints trades
           generate.cpp               gen:   writes a synthetic order stream
           bench.cpp                  bench: measures throughput and per-operation latency
data/      hand-written scenarios used as tests
tests/     test runner and expected outputs
```

## Input format

One command per line. Lines starting with `#` are comments.

```
A <id> <B|S> <price> <qty>    limit order
M <id> <B|S> <qty>            market order
C <id>                        cancel
```

Example (`data/trades.txt`):

```
# time priority: order 6 queues behind order 1 at the same price
A 6 S 1007 100
A 7 B 1007 250
```

## Building and running

The only requirements are a C++17 compiler and `make`.

```sh
make                               # debug build
make run FILE=data/trades.txt      # replay a scenario and print trades and the final book
make release CXX=g++               # pick the build mode and compiler
```

| Mode | Flags | Use |
|---|---|---|
| `debug` | `-O0 -g` | Development and debugging |
| `release` | `-O3 -DNDEBUG` | Benchmarking |
| `sanitize` | UBSan plus libc++ debug checks | Catching undefined behaviour and bad iterator use |
| `asan` | AddressSanitizer plus UBSan | Memory errors (Linux) |

Each mode builds into `build/<mode>/`.

## Testing

```sh
make test      # replay every data/*.txt and compare with tests/expected/
make stress    # run 200,000 generated orders under the sanitizer build
```

- **Golden tests:** the files in `data/` are hand-written scenarios covering partial fills, time priority, sweeping several price levels, market orders running out of liquidity, and cancels of filled orders. The expected output for each was checked by hand.
- **Stress test:** runs a large generated order stream and fails on any crash or sanitizer error.
- **CI:** every push runs both on Linux (`g++` and `clang++`, in debug, release, sanitize and AddressSanitizer mode) and on macOS.

## Benchmarking

```sh
make bench                         # 1,000,000 orders, seed 42
make bench BENCH_ORDERS=10000000   # a larger run
make bench BENCH_SEED=7            # a different but repeatable order stream
```

### The generated orders

`gen` writes a seeded, reproducible stream:
- about 45% cancels of live orders
- about 3% market orders
- the rest are limit orders, with prices clustered near a slowly drifting mid price and about 5% priced aggressively enough to cross the spread

On the default stream, the book stays two-sided with a 1-tick spread, about 70% of cancels hit a resting order, and roughly 170,000 trades occur.

### How it's measured

The order file is parsed into memory before any timing starts. The commands then run twice, each time on a fresh book:
1. **Throughput run:** the whole loop is timed once, with no per-operation clock reads.
2. **Latency run:** each operation is timed on its own, and the results are reported as mean, p50, p90, p99, p99.9 and maximum, separately for limit, market and cancel.

Both runs compute a checksum of their results. It confirms the two runs did identical work and stops the compiler from optimising the work away. The benchmark also reports the system clock's tick size, because latencies are rounded to it: about 41 ns on Apple Silicon.

### Results

_To be added._

## Roadmap

- [ ] Price levels in a flat array indexed by price tick, instead of `std::map`
- [ ] Intrusive order lists backed by a pre-allocated pool, instead of `std::list`
- [ ] Order lookup in a flat vector, instead of `std::unordered_map`
- [ ] No memory allocation on the matching path
- [ ] Order modify (a quantity reduction keeps queue priority)
- [ ] Immediate-or-cancel (IOC) and fill-or-kill (FOK) orders
- [ ] Replay real NASDAQ ITCH market data
