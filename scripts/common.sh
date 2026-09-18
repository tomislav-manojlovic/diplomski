#!/usr/bin/env bash

CPU=0

MACHINE="${1:-unknown}"

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

init_run() {
    benchmark="$1"
    type="$2"

    output_dir="results/raw/$MACHINE/$benchmark"
    mkdir -p "$output_dir"

    output_file="$output_dir/$type.txt"

    exec > >(tee "$output_file") 2>&1

    echo "Machine: $MACHINE"
    echo "CPU: $CPU"
    echo
}
