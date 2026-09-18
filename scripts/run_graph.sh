#!/usr/bin/env bash

source "$(dirname "$0")/common.sh"

init_run graph timing

n=1048576
degree=8
reps=5
source_vertex=0

for impl in adj csr csr_reordered; do
    echo "================ $impl ================"

    for r in {1..5}; do
        taskset -c "$CPU" \
            ./build/graph_benchmark \
            "$impl" "$n" "$degree" "$reps" "$source_vertex"
    done

    echo
done
