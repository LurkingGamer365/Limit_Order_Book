# Limit Order Book

[![CI](https://github.com/LurkingGamer365/Limit_Order_Book/actions/workflows/ci.yml/badge.svg)](https://github.com/LurkingGamer365/Limit_Order_Book/actions/workflows/ci.yml)

A limit order book and matching engine in C++17. It matches orders by price-time priority and supports limit orders, market orders and cancels. It also comes with a reproducible order generator, a latency benchmark, and tests that run in CI under sanitizers on Linux and macOS.

## Features

- **Price-time priority matching:** the best price fills first, and within a price the oldest order fills first.
- **Limit orders:** cross the spread as far as their limit price allows, then rest in the book.
- **Market orders:** fill at any price. Whatever can't be filled is discarded.
- **Cancels:** an order can be cancelled by id in O(1). Cancelling an order that has already been filled, or never existed, is rejected.
- **Partial fills:** an order can trade against several resting orders and price levels.
- **Price window:** a limit order priced outside the book's price window is rejected, never stored, so it can't index outside the price-level array.
- **Order id checks:** a limit order is rejected if its id is out of range or already in use by a resting order.

## Design

| Component | Data structure | Why |
|---|---|---|
| Price levels | `BookSide`: a `std::vector<Level>` indexed by `price - base` | Reaching a price is arithmetic, not a tree search. Each side tracks the index of its best level and how many levels are non-empty, so checking for an empty side and reading the best price are both O(1). |
| Orders at one price | `std::list<Order>` | A FIFO queue for time priority. An order can be removed from the middle in O(1). |
| Order lookup | `std::vector<OrderLocation>` indexed by order id | Maps an order id to its side, price, list iterator and whether it's still resting, so a cancel goes straight to the order. Sized once when the book is created, so there's no hashing, no memory allocation and no rebuilding while the book runs. |

| Operation | Cost |
|---|---|
| Add an order that doesn't trade | O(1) |
| Cancel | O(1), plus a scan to the next best level if it empties the best level |
| Match | O(1) per order filled, plus a scan to the next best level each time a level is used up |

The scans move one tick at a time towards worse prices. They're short in practice, because the levels near the best price are almost always occupied. When a side's last level empties, the side is marked empty without scanning.

`OrderBook` doesn't print anything: it returns `Trade`s to the caller, so the matching engine can be timed on its own.

**Order ids:** the lookup is indexed by order id, so ids must be dense, counting up from 0 or 1 without huge gaps. That's true within a trading session on a real exchange, and true of the generated orders. `lob` and `bench` size the book to the largest id in their input. A limit order is rejected if its id is outside that range, or if another order with the same id is still resting. Once an order has been filled or cancelled, its id can be used again.

**Price window:** the window is fixed when the book is created, and the default is prices 0 to 65,535. A limit order priced outside it is rejected, even one that would have traded completely without resting. That's similar to the price bands real exchanges use. Market orders and cancels have no price to check, so they're unaffected. Accepting out-of-window orders with an overflow structure is on the roadmap.

## Project layout

```
include/   order_book.h               the order book
           book_side.h                one side of the book: the price-level array
           types.h                    Order, Trade and Level
           parser.h                   the input parser
src/       order_book.cpp, book_side.cpp, parser.cpp
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

- **Golden tests:** the files in `data/` are hand-written scenarios covering partial fills, time priority, sweeping several price levels, market orders running out of liquidity, cancels of filled orders, prices on and just outside the edges of the price window, and the order id rules (a duplicate resting id, a negative id, and reusing a cancelled order's id). The expected output for each was checked by hand.
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

Apple M1 Pro, 16 GB, macOS 27.0.1, Apple clang 16.0.0, `-O3`, 1,000,000 orders, seed 42.

Each optimisation was measured against the version before it, in the same session, alternating between the two, 3 runs each. Each figure is the median of the 3 runs, and every version produces identical output on all 1,000,000 orders. Separate sessions ran under different conditions (the flat order index on battery power), so compare the numbers within a row, not between rows.

| Change | Throughput | Limit mean | Cancel mean | Limit max |
|---|---:|---:|---:|---:|
| Flat array price levels (replacing `std::map`) | 13.4 → **17.2** M orders/s | 88 → **68** ns | 68 → **52** ns | ~238 → ~247 µs |
| Flat order index (replacing `std::unordered_map`) | 16.0 → **34.5** M orders/s | 69 → **34** ns | 56 → **30** ns | ~240 → **~28 µs** |

**Current version** (flat array price levels, flat order index)

| Latency (ns) | Count | Mean | p50 | p90 | p99 | p99.9 | Max |
|---|---:|---:|---:|---:|---:|---:|---:|
| Limit | 520,446 | 34 | 42 | 42 | 83 | 166 | 27,500 |
| Market | 30,112 | 33 | 41 | 42 | 84 | 167 | 5,208 |
| Cancel | 449,442 | 30 | 41 | 42 | 84 | 250 | 28,000 |

The clock ticks every 41 ns on this machine, so p50 and p90 are rounded to multiples of it. Most operations now finish within a single tick, so the means and the throughput are better measures of the remaining differences. The p99.9 and max values vary noticeably from run to run.

**What each step showed**
- **Flat array price levels:** roughly halved the median latency of limit and market orders. The worst case for limit orders stayed at about 240 µs, which the experiment below traced to the order lookup map.
- **Flat order index:** roughly doubled throughput and cut the limit-order worst case from about 240 µs to about 28 µs. That's more than pre-sizing the map achieved, because `std::unordered_map` also allocates a separate node in memory for every order it stores and frees it when the order leaves. The vector does no allocation after the book is created.

<details>
<summary>Full latency tables for earlier versions</summary>

**Flat array price levels**

| Latency (ns) | Count | Mean | p50 | p90 | p99 | p99.9 | Max |
|---|---:|---:|---:|---:|---:|---:|---:|
| Limit | 520,446 | 68 | 42 | 84 | 167 | 667 | 246,791 |
| Market | 30,112 | 71 | 42 | 125 | 250 | 417 | 4,541 |
| Cancel | 449,442 | 52 | 42 | 84 | 125 | 417 | 18,542 |

**`std::map` price levels** (baseline)

| Latency (ns) | Count | Mean | p50 | p90 | p99 | p99.9 | Max |
|---|---:|---:|---:|---:|---:|---:|---:|
| Limit | 520,446 | 88 | 83 | 125 | 208 | 1,416 | 237,834 |
| Market | 30,112 | 78 | 83 | 166 | 291 | 792 | 10,833 |
| Cancel | 449,442 | 68 | 42 | 125 | 250 | 2,500 | 35,000 |

</details>

### Finding the worst case: pre-sizing the order map

This was an experiment to find what causes the ~240 µs worst case, not a change to the code. The id lookup map (`std::unordered_map`) was pre-sized with `orders.reserve(1 << 20)`, so it never has to grow during a run. Both versions ran alternately on the same 1,000,000 orders (on battery power, so compare these runs only with each other). Each figure is the median of 3 runs.

| | Throughput | Limit mean | Limit p99.9 | Limit max | Cancel max |
|---|---:|---:|---:|---:|---:|
| Current code | 16.4 M orders/s | 69 ns | 458 ns | 239,583 ns | 25,959 ns |
| Map pre-sized | **18.4 M orders/s** | **59 ns** | **416 ns** | **35,334 ns** | **24,750 ns** |

Both versions produce the same checksum, so they did identical work.

The worst case for limit orders fell from about 240 µs to about 35 µs. The spike came from the map growing: when an `unordered_map` fills past its capacity, it allocates a bigger table and re-inserts every entry at once, and with hundreds of thousands of live orders that takes hundreds of microseconds. That led to the flat order index above, which avoids the rebuild, the hashing and the per-order allocation.

## Roadmap

- [x] Price levels in a flat array indexed by price tick, instead of `std::map`
- [x] Reject orders priced outside the array window
- [ ] Accept orders priced outside the window, in an overflow structure
- [ ] Intrusive order lists backed by a pre-allocated pool, instead of `std::list`
- [x] Order lookup in a flat vector, instead of `std::unordered_map`
- [ ] No memory allocation on the matching path
- [ ] Order modify (a quantity reduction keeps queue priority)
- [ ] Immediate-or-cancel (IOC) and fill-or-kill (FOK) orders
- [ ] Replay real NASDAQ ITCH market data
