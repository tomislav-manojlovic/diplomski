from pathlib import Path
import re

import pandas as pd


ROOT = Path(__file__).resolve().parents[1]
RAW_DIR = ROOT / "results" / "raw"
SUMMARY_DIR = ROOT / "results" / "summary"

MACHINES = ["machine1", "machine2"]
CATEGORIES = ["matrix", "search", "hash", "graph"]

PERF_COLUMNS = [
    "cycles",
    "instructions",
    "cache_references",
    "cache_misses",
    "branches",
    "branch_misses",
]

KNOWN_SEARCH = {"binary", "bst", "eytzinger", "eytzinger_prefetch"}


# -----------------------------------------------------------------------------
# Pomocne funkcije
# -----------------------------------------------------------------------------

def number(text):
    """Vraca prvi broj iz stringa, ili None."""
    if text is None:
        return None
    match = re.search(r"[-+]?\d+(?:[.,]\d+)?", str(text))
    if not match:
        return None
    return float(match.group(0).replace(",", "."))


def integer(text):
    value = number(text)
    return int(value) if value is not None else None


def read_raw_files(directory):
    """Vraca (putanja, linije) za sve tekstualne fajlove u direktorijumu."""
    if not directory.exists():
        return []

    result = []
    for path in sorted(p for p in directory.rglob("*") if p.is_file()):
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        result.append((path, text.splitlines()))
    return result


def clean_header(text):
    return text.strip().strip("=- ").strip()


def perf_event(line, machine):
    """
    Parsira perf stat liniju.

    Na Intel hybrid masini perf ispisuje cpu_core/... i cpu_atom/....
    Posto je benchmark pinovan na CPU 0, za machine2 cuvamo cpu_core brojače.
    cpu_atom redovi se ignorisu.
    """
    match = re.match(r"^\s*([0-9]+)\s+(\S+)", line)
    if not match:
        return None

    value = int(match.group(1))
    event = match.group(2)

    if event.startswith("cpu_atom/"):
        return None

    if event.startswith("cpu_core/"):
        event = event[len("cpu_core/"):]
        if event.endswith("/"):
            event = event[:-1]

    event = re.sub(r":[ukhHGpP]+$", "", event)

    mapping = {
        "cycles": "cycles",
        "instructions": "instructions",
        "cache-references": "cache_references",
        "cache-misses": "cache_misses",
        "branches": "branches",
        "branch-instructions": "branches",
        "branch-misses": "branch_misses",
    }

    key = mapping.get(event)
    if key is None:
        return None
    return key, value


def parse_key_value(line):
    if ":" not in line:
        return None, None
    key, value = line.split(":", 1)
    return key.strip().lower(), value.strip()


def normalize_matrix_algorithm(record):
    # Najpouzdanije je ako je algoritam izvucen iz perf komande.
    if record.get("algorithm"):
        return record["algorithm"]

    order = str(record.get("order", "")).strip().lower()
    hint = f"{record.get('_header', '')} {record.get('_subheader', '')}".lower()

    if order == "ikj" or re.search(r"\bikj\b", hint):
        return "ikj"

    if order.startswith("blocked") or "blocked" in hint:
        size = record.get("block_size")
        if size is None:
            m = re.search(r"blocked[^0-9]*(\d+)", hint)
            size = int(m.group(1)) if m else None
        return f"blocked_{size}" if size else "blocked"

    if order.startswith("recursive") or "recursive" in hint:
        cutoff = record.get("cutoff")
        if cutoff is None:
            m = re.search(r"recursive[^0-9]*(\d+)", hint)
            cutoff = int(m.group(1)) if m else None
        return f"recursive_{cutoff}" if cutoff else "recursive"

    return order or None


def normalize_search_algorithm(record):
    if record.get("algorithm"):
        return record["algorithm"]

    for text in (record.get("_header", ""), record.get("_subheader", "")):
        value = clean_header(text).lower().replace(" ", "_")
        if value in KNOWN_SEARCH:
            return value
    return None


def enrich_from_perf_command(record, line, category):
    if category == "matrix":
        m = re.search(r"matrix_loop_order\s+(\d+)\s+(\w+)(?:\s+(\d+))?", line)
        if m:
            record["N"] = int(m.group(1))
            alg = m.group(2)
            parameter = int(m.group(3)) if m.group(3) else None
            if alg == "blocked" and parameter:
                record["algorithm"] = f"blocked_{parameter}"
            elif alg == "recursive" and parameter:
                record["algorithm"] = f"recursive_{parameter}"
            else:
                record["algorithm"] = alg

    elif category == "search":
        m = re.search(r"search_benchmark\s+(\w+)\s+(\d+)", line)
        if m:
            record["algorithm"] = m.group(1)
            record["N"] = int(m.group(2))

    elif category == "hash":
        m = re.search(
            r"hash_benchmark\s+(\w+)\s+(\d+)\s+\d+\s+([0-9.]+)\s+(hit|miss)",
            line,
        )
        if m:
            record["algorithm"] = m.group(1)
            record["N"] = int(m.group(2))
            record["load_factor"] = round(float(m.group(3)), 2)
            record["query_type"] = m.group(4)

    elif category == "graph":
        m = re.search(r"graph_benchmark\s+(\w+)\s+(\d+)\s+(\d+)", line)
        if m:
            record["algorithm"] = m.group(1)
            record["N"] = int(m.group(2))
            record["degree"] = int(m.group(3))


# -----------------------------------------------------------------------------
# Parsiranje raw izlaza
# -----------------------------------------------------------------------------

def parse_category(machine, category):
    directory = RAW_DIR / machine / category
    records = []

    for source_path, lines in read_raw_files(directory):
        header = ""
        subheader = ""
        record = None

        def finish_record():
            nonlocal record
            if record is None:
                return

            record["machine"] = machine
            record["_source"] = str(source_path.relative_to(ROOT))

            if category == "matrix":
                record["algorithm"] = normalize_matrix_algorithm(record)
            elif category == "search":
                record["algorithm"] = normalize_search_algorithm(record)

            # Normalizacija load factor-a zbog ispisa tipa 0.949999.
            if record.get("load_factor") is not None:
                record["load_factor"] = round(float(record["load_factor"]), 2)

            records.append(record)
            record = None

        for line in lines:
            stripped = line.strip()

            # Glavni header: ================ binary ================
            m = re.match(r"^=+\s*(.*?)\s*=+$", stripped)
            if m:
                finish_record()
                header = clean_header(m.group(1))
                subheader = ""
                continue

            # Pod-header: ----- ikj ----- ili ----- chaining -----
            m = re.match(r"^-{3,}\s*(.*?)\s*-{3,}$", stripped)
            if m:
                text = clean_header(m.group(1))
                if not re.fullmatch(r"RUN\s+\d+", text, flags=re.IGNORECASE):
                    subheader = text
                continue

            # Pocetak novog benchmark zapisa.
            starts_record = False
            if category in {"matrix", "search"} and re.match(r"^N:\s*\d+", stripped):
                starts_record = True
            elif category in {"hash", "graph"} and stripped.startswith("implementation:"):
                starts_record = True

            if starts_record:
                finish_record()
                record = {
                    "_header": header,
                    "_subheader": subheader,
                }

            if record is None:
                continue

            if "Performance counter stats for" in line:
                enrich_from_perf_command(record, line, category)

            event = perf_event(line, machine)
            if event:
                key, value = event
                record[key] = value
                continue

            key, value = parse_key_value(line)
            if key is None:
                continue

            if key == "n":
                record["N"] = integer(value)
            elif key == "vertices":
                record["N"] = integer(value)
            elif key == "degree":
                record["degree"] = integer(value)
            elif key == "implementation":
                record["algorithm"] = value.strip()
            elif key == "order":
                record["order"] = value.strip()
            elif key in {"block size", "block_size", "block"}:
                record["block_size"] = integer(value)
            elif key == "cutoff":
                record["cutoff"] = integer(value)
            elif key == "time":
                record["time_s"] = number(value)
            elif key == "time/run":
                record["time_per_run_s"] = number(value)
            elif key == "gflop/s":
                record["gflops"] = number(value)
            elif key == "ns/query":
                record["ns_per_query"] = number(value)
            elif key == "ns/edge":
                record["ns_per_edge"] = number(value)
            elif key == "load factor":
                record["load_factor"] = number(value)
            elif key == "query type":
                record["query_type"] = value.strip().lower()

        finish_record()

    return pd.DataFrame(records)


# -----------------------------------------------------------------------------
# Agregacija
# -----------------------------------------------------------------------------

def mean_summary(df, group_columns, value_columns):
    columns = group_columns + value_columns
    available = [c for c in columns if c in df.columns]
    if df.empty or any(c not in df.columns for c in group_columns):
        return pd.DataFrame(columns=columns + ["runs"])

    numeric = [c for c in value_columns if c in df.columns]
    result = (
        df.groupby(group_columns, dropna=False, as_index=False)[numeric]
        .mean(numeric_only=True)
    )
    runs = df.groupby(group_columns, dropna=False).size().reset_index(name="runs")
    result = result.merge(runs, on=group_columns, how="left")

    ordered = group_columns + ["runs"] + numeric
    return result[ordered]


def detailed_summary(df, category):
    """Detaljni CSV: samo run-ovi koji imaju perf brojače."""
    if df.empty or "cycles" not in df.columns:
        return pd.DataFrame()

    perf = df[df["cycles"].notna()].copy()
    if perf.empty:
        return pd.DataFrame()

    if category == "matrix":
        group = ["N", "algorithm"]
        values = ["time_s", "gflops"] + PERF_COLUMNS
    elif category == "search":
        group = ["N", "algorithm"]
        values = ["time_s", "ns_per_query"] + PERF_COLUMNS
    elif category == "hash":
        group = ["N", "load_factor", "query_type", "algorithm"]
        values = ["time_s", "ns_per_query"] + PERF_COLUMNS
    else:
        group = ["N", "degree", "algorithm"]
        values = ["time_per_run_s", "ns_per_edge"] + PERF_COLUMNS

    result = mean_summary(perf, group, values)

    if result.empty:
        return result

    # Perf brojače prikazujemo kao prosečne cele brojeve.
    for column in PERF_COLUMNS:
        if column in result.columns:
            result[column] = result[column].round().astype("Int64")

    result["ipc"] = result["instructions"] / result["cycles"]
    result["cache_miss_rate_pct"] = 100.0 * result["cache_misses"] / result["cache_references"]
    result["branch_miss_rate_pct"] = 100.0 * result["branch_misses"] / result["branches"]

    # Malo uredniji broj decimala u CSV-u.
    for column in [
        "time_s", "time_per_run_s", "gflops", "ns_per_query", "ns_per_edge",
        "ipc", "cache_miss_rate_pct", "branch_miss_rate_pct",
    ]:
        if column in result.columns:
            result[column] = result[column].round(6)

    return result


def comparison_summary(all_machines, category):
    """
    Time comparison koristi run-ove bez perf-a kad god postoje.
    Ako za neki test nema posebnih time-only run-ova, koristi perf run-ove kao fallback.
    """
    df = pd.concat(all_machines, ignore_index=True) if all_machines else pd.DataFrame()
    if df.empty:
        return pd.DataFrame()

    if category == "matrix":
        group = ["N", "machine", "algorithm"]
        time_column = "time_s"
    elif category == "search":
        group = ["N", "machine", "algorithm"]
        time_column = "time_s"
    elif category == "hash":
        group = ["N", "load_factor", "query_type", "machine", "algorithm"]
        time_column = "time_s"
    else:
        group = ["N", "degree", "machine", "algorithm"]
        time_column = "time_per_run_s"

    if time_column not in df.columns:
        return pd.DataFrame(columns=group + [time_column])

    rows = []
    for keys, part in df.groupby(group, dropna=False):
        if not isinstance(keys, tuple):
            keys = (keys,)

        # Po mogucstvu uzimamo samo run-ove bez perf brojača.
        if "cycles" in part.columns:
            plain = part[part["cycles"].isna()]
            chosen = plain if not plain.empty else part
        else:
            chosen = part

        row = dict(zip(group, keys))
        row[time_column] = round(float(chosen[time_column].mean()), 6)
        rows.append(row)

    return pd.DataFrame(rows).sort_values(group).reset_index(drop=True)


def save_csv(df, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    df.to_csv(path, index=False)
    print(f"Saved: {path.relative_to(ROOT)}")


# -----------------------------------------------------------------------------
# Main
# -----------------------------------------------------------------------------

def main():
    parsed = {category: {} for category in CATEGORIES}

    for category in CATEGORIES:
        for machine in MACHINES:
            df = parse_category(machine, category)
            parsed[category][machine] = df

            detailed = detailed_summary(df, category)
            save_csv(detailed, SUMMARY_DIR / category / f"{machine}.csv")

        comparison = comparison_summary(
            [parsed[category][machine] for machine in MACHINES],
            category,
        )
        save_csv(comparison, SUMMARY_DIR / category / "time_comparison.csv")

    print("\nDone. Summary fajlovi su u results/summary/.")


if __name__ == "__main__":
    main()

