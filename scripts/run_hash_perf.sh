#!/usr/bin/env bash

source "$(dirname "$0")/common.sh"

init_run hash perf

n=1048576
q=5000000
lf=0.95

for impl in chaining linear robin_hood; do
    echo "================ $impl / miss ================"

    for r in {1..3}; do
        echo "----- RUN $r -----"

        perf stat \
            -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
            taskset -c "$CPU" \
            ./build/hash_benchmark "$impl" "$n" "$q" "$lf" miss

        echo
    done
done
