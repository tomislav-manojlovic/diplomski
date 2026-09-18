#!/usr/bin/env bash

source "$(dirname "$0")/common.sh"

init_run matrix perf

n=3072

for impl in ikj blocked recursive; do
    echo "================ $impl ================"

    for r in {1..3}; do
        echo "----- RUN $r -----"

        if [ "$impl" = "ikj" ]; then
            perf stat \
                -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
                taskset -c "$CPU" \
                ./build/matrix_loop_order "$n" ikj

        elif [ "$impl" = "blocked" ]; then
            perf stat \
                -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
                taskset -c "$CPU" \
                ./build/matrix_loop_order "$n" blocked 64

        else
            perf stat \
                -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
                taskset -c "$CPU" \
                ./build/matrix_loop_order "$n" recursive 64
        fi

        echo
    done
done
