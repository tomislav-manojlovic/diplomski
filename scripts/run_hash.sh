#!/usr/bin/env bash

source "$(dirname "$0")/common.sh"

init_run hash timing

n=1048576
q=5000000

for lf in 0.70 0.95; do
    echo "================ LF=$lf / HIT ================"

    for impl in chaining linear robin_hood; do
        echo "----- $impl -----"

        for r in {1..5}; do
            taskset -c "$CPU" \
                ./build/hash_benchmark "$impl" "$n" "$q" "$lf" hit
        done

        echo
    done
done

lf=0.95

echo "================ LF=$lf / MISS ================"

for impl in chaining linear robin_hood; do
    echo "----- $impl -----"

    for r in {1..5}; do
        taskset -c "$CPU" \
            ./build/hash_benchmark "$impl" "$n" "$q" "$lf" miss
    done

    echo
done
