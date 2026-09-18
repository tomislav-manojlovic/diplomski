#!/usr/bin/env bash

source "$(dirname "$0")/common.sh"

init_run search timing

n=1048576
q=5000000

for impl in binary bst eytzinger eytzinger_prefetch; do
    echo "================ $impl ================"

    for r in {1..5}; do
        taskset -c "$CPU" \
            ./build/search_benchmark "$impl" "$n" "$q"
    done

    echo
done
