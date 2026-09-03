#!/usr/bin/env python3
"""Aggregate and render compact recovery-coverage JSON v3 rows.

The probe owns leaf alignment, semantic text comparison, and expected-difference
classification. This script deliberately knows only how to add the probe's structured
counts and render them as plain-text tables.
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable, Iterator

TEXT_KINDS = ("known encoding glitch", "whitespace", "font", "size", "effects",
    "added font info", "empty part-name template", "missing run", "unresolved font", "other")
CODA_EPOCH = "coda-banner"
EPOCH_ORDER = (CODA_EPOCH, "uncompressed", "dcl", "zlib")
PROGRESS_INTERVAL_SECONDS = 1.9
VALUE_WIDTH = 60


@dataclass
class ClassStats:
    same: int = 0
    coda_expected: int = 0
    expected: int = 0
    unexpected: int = 0
    source_only: int = 0
    source_only_part: int = 0
    companion_only: int = 0

    def add(self, values: list[int], epoch: str) -> None:
        if len(values) not in (5, 6):
            raise ValueError("comparison class counts must contain five or six values")
        self.same += values[0]
        if epoch == CODA_EPOCH:
            self.coda_expected += values[1]
        self.expected += values[1]
        self.unexpected += values[2]
        self.source_only += values[3]
        if len(values) == 6:
            self.source_only_part += values[4]
            self.companion_only += values[5]
        else:
            self.companion_only += values[4]

    def values(self) -> list[int]:
        return [self.same, self.expected, self.unexpected,
            self.source_only, self.source_only_part, self.companion_only]


def read_rows(path: Path, show_progress: bool) -> Iterator[dict[str, Any]]:
    total_bytes = path.stat().st_size
    consumed = rows = companions = 0
    started = time.monotonic()
    next_progress = started + PROGRESS_INTERVAL_SECONDS

    def display() -> None:
        elapsed = time.monotonic() - started
        percent = 100.0 if not total_bytes else consumed * 100.0 / total_bytes
        rate = 0.0 if not elapsed else rows / elapsed
        print(f"Processed {rows} row(s), {companions} companion(s), "
            f"{percent:.1f}% of input in {elapsed:.1f}s ({rate:.1f} rows/s)",
            file=sys.stderr, flush=True)

    with path.open("rb") as handle:
        for line in handle:
            consumed += len(line)
            if not line.strip():
                continue
            row = json.loads(line)
            if row.get("schema") != 3:
                raise ValueError("recovery coverage report requires compact JSON schema 3")
            rows += 1
            companions += bool(row.get("companion"))
            yield row
            if show_progress and (rows == 1 or time.monotonic() >= next_progress):
                display()
                next_progress = time.monotonic() + PROGRESS_INTERVAL_SECONDS
    if show_progress:
        display()


def table_widths(headers: list[str], rows: Iterable[list[str]]) -> list[int]:
    widths = [len(value) for value in headers]
    for row in rows:
        for index, value in enumerate(row):
            widths[index] = max(widths[index], len(value))
    return widths


def print_table(title: str, headers: list[str], rows: Iterable[list[str]],
        widths: list[int] | None = None) -> None:
    rows = list(rows)
    print(f"\n{title}\n{'=' * len(title)}")
    if not rows:
        print("(none)")
        return
    widths = widths or table_widths(headers, rows)
    def line(cells: list[str]) -> str:
        return "  ".join(value.ljust(widths[index]) for index, value in enumerate(cells))
    print(line(headers))
    print("  ".join("-" * width for width in widths))
    for row in rows:
        print(line(row))


def truncate(value: Any) -> str:
    rendered = json.dumps(value, ensure_ascii=False)
    return rendered if len(rendered) <= VALUE_WIDTH else rendered[:VALUE_WIDTH - 1] + "…"


def ordered_epochs(names: Iterable[str]) -> list[str]:
    names = set(names)
    return [name for name in EPOCH_ORDER if name in names] \
        + sorted(names - set(EPOCH_ORDER))


def report(rows: Iterable[dict[str, Any]], max_unexpected: int) -> bool:
    row_count = companion_count = 0
    statuses: Counter[str] = Counter()
    epochs: Counter[str] = Counter()
    companion_statuses: Counter[str] = Counter()
    failures: Counter[str] = Counter()
    failure_examples: dict[str, str] = {}
    classes: dict[str, dict[str, ClassStats]] = {}
    expected_by_epoch: dict[str, Counter[str]] = {}
    transformations: Counter[str] = Counter()
    substitutions: Counter[str] = Counter()
    text: Counter[tuple[str, str]] = Counter()
    unexpected_examples: list[tuple[str, list[Any]]] = []
    text_examples: list[tuple[str, list[Any]]] = []

    for row in rows:
        row_count += 1
        status = row.get("status", "?")
        statuses[status] += 1
        if status == "ok":
            epochs[row.get("epoch", "-")] += 1
        else:
            message = row.get("error", "")
            failures[message] += 1
            failure_examples.setdefault(message, row.get("corpus_id", "?"))
        companion = row.get("companion")
        if not companion:
            continue
        companion_count += 1
        companion_statuses[companion.get("status", "?")] += 1
        comparison = row.get("comparison")
        if not comparison:
            continue
        for pool, pool_classes in comparison.get("classes", {}).items():
            aggregate = classes.setdefault(pool, {})
            for name, values in pool_classes.items():
                aggregate.setdefault(name, ClassStats()).add(
                    values, row.get("epoch", "-"))
        expected_by_epoch.setdefault(row.get("epoch", "-"), Counter()).update(
            comparison.get("expected", {}))
        transformations.update(comparison.get("transformations", {}))
        substitutions.update(comparison.get("font_substitutions", {}))
        for class_name, counts in comparison.get("text", {}).items():
            text.update({(class_name, kind): count for kind, count in counts.items()})
        unexpected_examples.extend((row.get("corpus_id", "?"), item)
            for item in comparison.get("unexpected", []))
        text_examples.extend((row.get("corpus_id", "?"), item)
            for item in comparison.get("text_examples", []))

    if not row_count:
        return False
    print(f"\n{row_count} document(s): {statuses['ok']} ok, {statuses['error']} error")
    print_table("Epoch (ok documents)", ["epoch", "count"],
        ([name, str(count)] for name, count in epochs.most_common()))
    print_table("Failure reasons", ["count", "example", "message"],
        ([str(count), failure_examples[message], message]
            for message, count in failures.most_common()))
    if not companion_count:
        print("\nNo rows carried a companion.")
        return True
    print(f"\n{companion_count} row(s) with a companion: "
        f"{companion_statuses['ok']} ok, {companion_statuses['error']} error")

    substitution_rows = []
    for pair, count in substitutions.most_common():
        source, companion = pair.split("\t", 1)
        substitution_rows.append([source, companion, str(count)])
    print_table("SetFont substitutions (also counted as expected differences)",
        ["source font", "companion font", "count"], substitution_rows)
    print_table("Recognized companion transformations", ["transformation", "count"],
        ([name, str(count)] for name, count in transformations.most_common()))
    expected_epochs = ordered_epochs(set(epochs) | set(expected_by_epoch))
    expected_rules = sorted({rule for counts in expected_by_epoch.values() for rule in counts})
    expected_rows = [[rule,
        *[str(expected_by_epoch.get(epoch, Counter())[rule]) for epoch in expected_epochs],
        str(sum(expected_by_epoch.get(epoch, Counter())[rule] for epoch in expected_epochs))]
        for rule in expected_rules]
    epoch_totals = [sum(expected_by_epoch.get(epoch, Counter()).values())
        for epoch in expected_epochs]
    expected_rows.append(["TOTAL", *map(str, epoch_totals), str(sum(epoch_totals))])
    print_table("Expected differences", ["rule", *expected_epochs, "total"], expected_rows)

    headers = ["class", "same", "coda-expected-diff", "expected-diff", "unexpected-diff",
        "reader-only-score", "reader-only-part", "companion-only", "total"]
    pool_rows: dict[str, list[list[str]]] = {}
    ordered_pools = [pool for pool in ("options", "others", "details", "texts")
        if pool in classes]
    ordered_pools.extend(sorted(set(classes) - set(ordered_pools)))
    for pool in ordered_pools:
        class_rows = []
        for name, stats in sorted(classes[pool].items()):
            values = stats.values()
            class_rows.append([name, str(values[0]), str(stats.coda_expected),
                *map(str, values[1:]), str(sum(values))])
        if class_rows:
            class_rows.append(["TOTAL", *[str(sum(int(row[index]) for row in class_rows))
                for index in range(1, len(headers))]])
        pool_rows[pool] = class_rows
    shared_widths = table_widths(headers,
        (row for values in pool_rows.values() for row in values))
    shared_widths[1] = max(shared_widths[1], 10)
    print("\nCompanion-comparison columns count leaves, except that each classified "
        "Enigma-text finding counts separately.")
    printed = False
    for pool, values in pool_rows.items():
        if not values:
            continue
        if printed:
            print("\n" + "-" * 80)
        print_table(f"{pool} pool companion comparison", headers, values, shared_widths)
        printed = True
    all_classes = [stats for pool_classes in classes.values() for stats in pool_classes.values()]
    totals = [sum(getattr(value, field) for value in all_classes)
        for field in ("same", "expected", "unexpected", "source_only", "source_only_part",
            "companion_only")]
    coda_expected_total = sum(value.coda_expected for value in all_classes)
    expected_coda_total = sum(expected_by_epoch.get(CODA_EPOCH, Counter()).values())
    if coda_expected_total != expected_coda_total:
        raise ValueError("Coda class expected-difference total does not match rule total")
    grand = ["ALL POOLS", str(totals[0]), str(coda_expected_total),
        *map(str, totals[1:]), str(sum(totals))]
    print("\n" + "  ".join(value.ljust(shared_widths[index])
        for index, value in enumerate(grand)))

    text_classes = sorted(classes.get("texts", {}))
    text_rows = [[class_name, *[str(text[(class_name, kind)]) for kind in TEXT_KINDS],
        str(sum(text[(class_name, kind)] for kind in TEXT_KINDS))]
        for class_name in text_classes]
    text_rows.append(["TOTAL", *[str(sum(int(row[index]) for row in text_rows))
        for index in range(1, len(TEXT_KINDS) + 2)]])
    print_table("Enigma-text difference findings (one text may contribute more than one)",
        ["text type", "encoding", "whitespace", "font", "size", "effects",
            "added font info", "empty part-name", "missing run", "unresolved font", "other",
            "total"], text_rows)

    for kind, title in (("missing run", "Missing-run Enigma-text differences"),
            ("other", "Unclassified Enigma-text differences")):
        selected = [(corpus_id, item) for corpus_id, item in text_examples
            if len(item) > 3 and item[3] == kind]
        if selected:
            shown = selected[:max_unexpected]
            print_table(f"{title} (first {len(shown)} of {len(selected)})",
                ["kind", "corpus_id", "path", "source", "companion"],
                ([kind, corpus_id, item[0], truncate(item[1]), truncate(item[2])]
                    for corpus_id, item in shown))
    if unexpected_examples:
        shown = unexpected_examples[:max_unexpected]
        print_table(f"Non-text unexpected differences (first {len(shown)} of "
            f"{len(unexpected_examples)})",
            ["corpus_id", "path", "source", "companion", "origin"],
            ([corpus_id, item[0], truncate(item[1]), truncate(item[2]),
                 item[3] if len(item) > 3 else ""]
                for corpus_id, item in shown))
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("jsonl", type=Path, help="compact recovery_coverage_probe JSONL")
    parser.add_argument("--max-unexpected", type=int, default=80)
    parser.add_argument("--progress", action="store_true")
    args = parser.parse_args()
    try:
        if not report(read_rows(args.jsonl, args.progress), args.max_unexpected):
            print("no rows in input", file=sys.stderr)
            return 1
    except (OSError, ValueError, json.JSONDecodeError) as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
