# Build modes:
#   make            debug build (same as `make debug`)
#   make debug      no optimisation, debug symbols - use with a debugger
#   make release    fully optimised - use for timing/benchmarks
#   make sanitize   UBSan + libc++ bounds/iterator checks - use when hunting crashes
#   make asan       AddressSanitizer (memory errors); currently broken on this
#                   macOS version, works on Linux
#   make all-modes  build every mode above except asan
#   make run        build and run (MODE=release etc. to pick a mode,
#                   FILE=data/trades.txt to pick the input)
#   make test       build and check every data/*.txt against tests/expected/
#                   (TEST_MODE=release etc. to pick a mode; default sanitize)
#   make golden     regenerate tests/expected/ from the current output -
#                   only after checking the new output is correct
#   make clean      delete build/
#
# Pick a compiler with CXX, e.g. `make release CXX=g++`.
# Each mode builds into its own folder: build/<mode>/lob

CXX   := clang++
STD   := -std=c++17
WARN  := -Wall -Wextra -Wpedantic
SRCS  := $(wildcard src/*.cpp)
HDRS  := $(wildcard include/*.h)
INC   := -Iinclude
BUILD := build
MODE  ?= debug
FILE  ?= data/dummyBook.txt
TEST_MODE ?= sanitize

DEBUG_FLAGS    := -O0 -g
RELEASE_FLAGS  := -O3 -DNDEBUG
SANITIZE_FLAGS := -O1 -g -fno-omit-frame-pointer -fsanitize=undefined \
                  -fno-sanitize-recover=all \
                  -D_LIBCPP_HARDENING_MODE=_LIBCPP_HARDENING_MODE_DEBUG
ASAN_FLAGS     := -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined \
                  -fno-sanitize-recover=all

.PHONY: all debug release sanitize asan all-modes run test golden clean

all: debug

debug:     $(BUILD)/debug/lob
release:   $(BUILD)/release/lob
sanitize:  $(BUILD)/sanitize/lob
asan:      $(BUILD)/asan/lob
all-modes: debug release sanitize

$(BUILD)/debug/lob:    MODE_FLAGS := $(DEBUG_FLAGS)
$(BUILD)/release/lob:  MODE_FLAGS := $(RELEASE_FLAGS)
$(BUILD)/sanitize/lob: MODE_FLAGS := $(SANITIZE_FLAGS)
$(BUILD)/asan/lob:     MODE_FLAGS := $(ASAN_FLAGS)

$(BUILD)/%/lob: $(SRCS) $(HDRS) Makefile
	@mkdir -p $(@D)
	$(CXX) $(STD) $(WARN) $(INC) $(MODE_FLAGS) $(SRCS) -o $@

run: $(BUILD)/$(MODE)/lob
	./$< $(FILE)

test: $(BUILD)/$(TEST_MODE)/lob
	./tests/run_tests.sh $<

golden: $(BUILD)/release/lob
	./tests/run_tests.sh $< --update

clean:
	rm -rf $(BUILD)
