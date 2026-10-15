#!/usr/bin/env bash
# Runs the program on every data/*.txt and compares its output with
# tests/expected/<name>.out.
# Usage: tests/run_tests.sh <binary> [--update]
#   --update  overwrite the expected outputs with the current output

set -u

bin="$1"
update="${2:-}"
status=0
actual="$(mktemp)"
trap 'rm -f "$actual"' EXIT

for input in data/*.txt; do
    name="$(basename "$input" .txt)"
    expected="tests/expected/$name.out"

    "$bin" "$input" > "$actual" 2>&1
    code=$?
    if [ $code -ne 0 ]; then
        echo "FAIL $name: exited with status $code"
        cat "$actual"
        status=1
        continue
    fi

    if [ "$update" = "--update" ]; then
        cp "$actual" "$expected"
        echo "UPDATED $name"
    elif [ ! -f "$expected" ]; then
        echo "FAIL $name: missing $expected (run 'make golden' to create it)"
        status=1
    elif diff -u "$expected" "$actual"; then
        echo "PASS $name"
    else
        echo "FAIL $name: output differs from $expected"
        status=1
    fi
done

exit $status
