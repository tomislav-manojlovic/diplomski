#!/usr/bin/env python3

import csv
from pathlib import Path
from collections import defaultdict
from statistics import mean

RAW = Path("results/raw")
OUT = Path("results/summary")
OUT.mkdir(parents=True, exist_ok=True)


def parse_blocks(path, first_key):
    blocks = []
    current = {}
    section = ""

    with open(path) as f:
        for line in f:
            line = line.strip()

            if line.startswith("================"):
                section = line.replace("=", "").strip()
                continue

            if ":" not in line:
                continue

            key, value = line.split(":", 1)
            key = key.strip()
            value = value.strip()

            if key == first_key and current:
                blocks.append(current)
                current = {}

            current[key] = value
            current["section"] = section

    if current:
        blocks.append(current)

    return blocks


def write_csv(filename, header, rows):
    with open(OUT / filename, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(header)
        writer.writerows(rows)


def number(value):
    return float(value.split()[0])


machines = [p for p in RAW.iterdir() if p.is_dir()]


# --------------------------------------------------
# Matrice
# --------------------------------------------------

data = defaultdict(lambda: {"time": [], "gflops": []})

for machine in machines:
    path = machine / "matrix" / "timing.txt"

    for b in parse_blocks(path, "N"):
        if "order" not in b or "time" not in b:
            continue

        impl = b["order"]

        if impl == "blocked":
            impl = "blocked_64"
        elif impl == "recursive":
            impl = "recursive_64"

        key = (machine.name, int(b["N"]), impl)

        data[key]["time"].append(number(b["time"]))
        data[key]["gflops"].append(number(b["GFLOP/s"]))

rows = []

for key, values in sorted(data.items()):
    machine, n, impl = key

    rows.append([
        machine,
        n,
        impl,
        len(values["time"]),
        mean(values["time"]),
        mean(values["gflops"])
    ])

write_csv(
    "matrix.csv",
    ["machine", "N", "implementation", "runs",
     "avg_time_s", "avg_gflops"],
    rows
)


# --------------------------------------------------
# Pretraga
# --------------------------------------------------

data = defaultdict(lambda: {"time": [], "ns": []})

for machine in machines:
    path = machine / "search" / "timing.txt"

    for b in parse_blocks(path, "N"):
        if "time" not in b or "ns/query" not in b:
            continue

        key = (
            machine.name,
            int(b["N"]),
            b["section"]
        )

        data[key]["time"].append(number(b["time"]))
        data[key]["ns"].append(number(b["ns/query"]))

rows = []

for key, values in sorted(data.items()):
    machine, n, impl = key

    rows.append([
        machine,
        n,
        impl,
        len(values["time"]),
        mean(values["time"]),
        mean(values["ns"])
    ])

write_csv(
    "search.csv",
    ["machine", "N", "implementation", "runs",
     "avg_time_s", "avg_ns_query"],
    rows
)


# --------------------------------------------------
# Hash 
# --------------------------------------------------

data = defaultdict(lambda: {"time": [], "ns": []})

for machine in machines:
    path = machine / "hash" / "timing.txt"

    for b in parse_blocks(path, "implementation"):
        if "implementation" not in b or "time" not in b:
            continue

        lf = round(float(b["load factor"]), 2)

        key = (
            machine.name,
            int(b["N"]),
            lf,
            b["query type"],
            b["implementation"]
        )

        data[key]["time"].append(number(b["time"]))
        data[key]["ns"].append(number(b["ns/query"]))

rows = []

for key, values in sorted(data.items()):
    machine, n, lf, query_type, impl = key

    rows.append([
        machine,
        n,
        lf,
        query_type,
        impl,
        len(values["time"]),
        mean(values["time"]),
        mean(values["ns"])
    ])

write_csv(
    "hash.csv",
    ["machine", "N", "load_factor", "query_type",
     "implementation", "runs", "avg_time_s", "avg_ns_query"],
    rows
)


# --------------------------------------------------
# Grafovi 
# --------------------------------------------------

data = defaultdict(lambda: {"time": [], "ns": []})

for machine in machines:
    path = machine / "graph" / "timing.txt"

    for b in parse_blocks(path, "implementation"):
        if "implementation" not in b or "time/run" not in b:
            continue

        key = (
            machine.name,
            int(b["vertices"]),
            int(b["degree"]),
            b["implementation"]
        )

        data[key]["time"].append(number(b["time/run"]))
        data[key]["ns"].append(number(b["ns/edge"]))

rows = []

for key, values in sorted(data.items()):
    machine, n, degree, impl = key

    rows.append([
        machine,
        n,
        degree,
        impl,
        len(values["time"]),
        mean(values["time"]),
        mean(values["ns"])
    ])

write_csv(
    "graph.csv",
    ["machine", "N", "degree", "implementation",
     "runs", "avg_time_run_s", "avg_ns_edge"],
    rows
)
