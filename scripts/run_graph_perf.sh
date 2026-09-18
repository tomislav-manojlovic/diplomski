#!/usr/bin/env bash

source "$(dirname "$0")/common.sh"

init_run graph perf

n=1048576
degree=8
reps=5
source_vertex=0

for impl in csr csr_reordered; do
    echo "================ $impl ================"

    for r in {1..3}; do
        echo "----- RUN $r -----"

        perf stat \
            -e cycles,instructions,cache-references,cache-misses,branches,branch-misses \
            taskset -c "$CPU" \
            ./build/graph_benchmark \
            "$impl" "$n" "$degree" "$reps" "$source_vertex"

        echo
    done
done
