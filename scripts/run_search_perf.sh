#!/usr/bin/env bash

source "$(dirname "$0")/common.sh"

init_run search perf

n=1048576
q=5000000

for impl in binary bst eytzinger eytzinger_prefetch; do
    echo "================ $impl ================"

    for r in {1..3}; do
        echo "----- RUN $r -----"

        perf stat \
            -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
            taskset -c "$CPU" \
            ./build/search_benchmark "$impl" "$n" "$q"

        echo
    done
done
