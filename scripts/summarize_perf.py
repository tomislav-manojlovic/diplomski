#!/usr/bin/env python3

import csv
import re
from pathlib import Path
from collections import defaultdict
from statistics import mean

RAW = Path("results/raw")
OUT = Path("results/summary")
OUT.mkdir(parents=True, exist_ok=True)

metrics = [
    "cycles",
    "instructions",
    "cache-references",
    "cache-misses",
    "branches",
    "branch-misses"
]

number_re = re.compile(
    r"^\s*([0-9,]+)\s+"
    r"(?:(cpu_core|cpu_atom)/)?"
    r"(cycles|instructions|cache-references|cache-misses|branches|branch-misses)"
)


def parse_perf(path):
    results = []
    section = ""
    current = {}

    with open(path) as f:
        for line in f:
            stripped = line.strip()

            if stripped.startswith("================"):
                if current:
                    results.append((section, current))
                    current = {}

                section = stripped.replace("=", "").strip()
                continue

            m = number_re.match(line)

            if not m:
                continue

            value = int(m.group(1).replace(",", ""))
            cpu_type = m.group(2)
            metric = m.group(3)

            # Na Intel procesoru ignorisemo cpu_atom.
            if cpu_type == "cpu_atom":
                continue

            # Novi cycles znaci novi perf run.
            if metric == "cycles" and "cycles" in current:
                results.append((section, current))
                current = {}

            current[metric] = value

    if current:
        results.append((section, current))

    return results


data = defaultdict(list)

for machine in RAW.iterdir():
    if not machine.is_dir():
        continue

    for benchmark in ["matrix", "search", "hash", "graph"]:
        path = machine / benchmark / "perf.txt"

        if not path.exists():
            continue

        for test, values in parse_perf(path):
            if all(m in values for m in metrics):

                if benchmark == "matrix":
                    if test == "blocked":
                        test = "blocked_64"
                    elif test == "recursive":
                        test = "recursive_64"

                data[(machine.name, benchmark, test)].append(values)


rows = []

for key, runs in sorted(data.items()):
    machine, benchmark, test = key

    cycles = mean(r["cycles"] for r in runs)
    instructions = mean(r["instructions"] for r in runs)
    cache_refs = mean(r["cache-references"] for r in runs)
    cache_misses = mean(r["cache-misses"] for r in runs)
    branches = mean(r["branches"] for r in runs)
    branch_misses = mean(r["branch-misses"] for r in runs)

    rows.append([
        machine,
        benchmark,
        test,
        len(runs),
        cycles,
        instructions,
        instructions / cycles,
        cache_refs,
        cache_misses,
        100 * cache_misses / cache_refs,
        branches,
        branch_misses,
        100 * branch_misses / branches
    ])


with open(OUT / "perf.csv", "w", newline="") as f:
    writer = csv.writer(f)

    writer.writerow([
        "machine",
        "benchmark",
        "test",
        "runs",
        "avg_cycles",
        "avg_instructions",
        "ipc",
        "avg_cache_references",
        "avg_cache_misses",
        "cache_miss_pct",
        "avg_branches",
        "avg_branch_misses",
        "branch_miss_pct"
    ])

    writer.writerows(rows)
