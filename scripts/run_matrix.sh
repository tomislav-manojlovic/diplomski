#!/usr/bin/env bash

source "$(dirname "$0")/common.sh"

init_run matrix timing

for n in 1024 3072; do
    echo "================ N=$n ================"

    echo "----- ikj -----"
    for r in {1..5}; do
        taskset -c "$CPU" ./build/matrix_loop_order "$n" ikj
    done

    echo "----- blocked 64 -----"
    for r in {1..5}; do
        taskset -c "$CPU" ./build/matrix_loop_order "$n" blocked 64
    done

    echo "----- recursive 64 -----"
    for r in {1..5}; do
        taskset -c "$CPU" ./build/matrix_loop_order "$n" recursive 64
    done

    echo
done
