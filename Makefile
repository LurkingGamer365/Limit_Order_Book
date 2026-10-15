# Build modes:
#   make            debug build (same as `make debug`)
#   make debug      no optimisation, debug symbols - use with a debugger
#   make release    fully optimised - use for timing/benchmarks
#   make sanitize   UBSan + libc++ bounds/iterator checks - use when hunting crashes
#   make asan       AddressSanitizer (memory errors); currently broken on this
#                   macOS version, works on Linux
#   make all-modes  build every mode above except asan
#
# Running:
#   make run        build and run (MODE=release etc. to pick a mode,
#                   FILE=data/trades.txt to pick the input)
#
# Testing:
#   make test       build and check every data/*.txt against tests/expected/
#                   (TEST_MODE=release etc. to pick a mode; default sanitize)
#   make golden     regenerate tests/expected/ from the current output -
#                   only after checking the new output is correct
#   make stress     run STRESS_ORDERS generated orders through TEST_MODE and
#                   fail on any crash or sanitizer error
#
# Benchmarking:
#   make bench      generate BENCH_ORDERS orders (default 1,000,000, cached in
#                   build/) and time every operation in a release build
#                   (BENCH_SEED=n for a different but repeatable order stream)
#
#   make clean      delete build/ (including generated order files)
#
# Pick a compiler with CXX, e.g. `make release CXX=g++`.
# Each mode builds into its own folder: build/<mode>/{lob,bench,gen}

CXX   := clang++
STD   := -std=c++17
WARN  := -Wall -Wextra -Wpedantic
LIB_SRCS := $(wildcard src/*.cpp)
HDRS  := $(wildcard include/*.h)
INC   := -Iinclude
BUILD := build
MODE  ?= debug
FILE  ?= data/dummyBook.txt
TEST_MODE ?= sanitize

BENCH_ORDERS  ?= 1000000
BENCH_SEED    ?= 42
BENCH_FILE    := $(BUILD)/orders_$(BENCH_ORDERS)_$(BENCH_SEED).txt
STRESS_ORDERS ?= 200000
STRESS_FILE   := $(BUILD)/orders_$(STRESS_ORDERS)_7.txt

DEBUG_FLAGS    := -O0 -g
RELEASE_FLAGS  := -O3 -DNDEBUG
SANITIZE_FLAGS := -O1 -g -fno-omit-frame-pointer -fsanitize=undefined \
                  -fno-sanitize-recover=all \
                  -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG
ASAN_FLAGS     := -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined \
                  -fno-sanitize-recover=all

.PHONY: all debug release sanitize asan all-modes run test golden stress bench clean
.SECONDARY:

all: debug

debug:     $(BUILD)/debug/lob
release:   $(BUILD)/release/lob
sanitize:  $(BUILD)/sanitize/lob
asan:      $(BUILD)/asan/lob
all-modes: debug release sanitize

$(BUILD)/debug/%:    MODE_FLAGS := $(DEBUG_FLAGS)
$(BUILD)/release/%:  MODE_FLAGS := $(RELEASE_FLAGS)
$(BUILD)/sanitize/%: MODE_FLAGS := $(SANITIZE_FLAGS)
$(BUILD)/asan/%:     MODE_FLAGS := $(ASAN_FLAGS)

# Compiles every .cpp listed in the rule's prerequisites into the target.
define compile
	@mkdir -p $(@D)
	$(CXX) $(STD) $(WARN) $(INC) $(MODE_FLAGS) $(filter %.cpp,$^) -o $@
endef

$(BUILD)/%/lob: apps/main.cpp $(LIB_SRCS) $(HDRS) Makefile
	$(compile)

$(BUILD)/%/bench: apps/bench.cpp $(LIB_SRCS) $(HDRS) Makefile
	$(compile)

$(BUILD)/%/gen: apps/generate.cpp Makefile
	$(compile)

run: $(BUILD)/$(MODE)/lob
	./$< $(FILE)

test: $(BUILD)/$(TEST_MODE)/lob
	./tests/run_tests.sh $<

golden: $(BUILD)/release/lob
	./tests/run_tests.sh $< --update

$(BUILD)/orders_%.txt: $(BUILD)/release/gen
	./$< $(word 1,$(subst _, ,$*)) $@ $(word 2,$(subst _, ,$*))

stress: $(BUILD)/$(TEST_MODE)/lob $(STRESS_FILE)
	./$< $(STRESS_FILE) > /dev/null
	@echo "stress: $(STRESS_ORDERS) orders ran cleanly under $(TEST_MODE)"

bench: $(BUILD)/release/bench $(BENCH_FILE)
	./$< $(BENCH_FILE)

clean:
	rm -rf $(BUILD)
