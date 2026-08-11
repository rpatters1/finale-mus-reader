#!/usr/bin/env python3
"""Survey imported FontOptions and compare available Finale 27 companions.

All path-bearing and per-fixture output is private. The input inventory is the
ignored corpus_inventory.csv produced by survey-a-corpus; this script never
modifies the corpus or its exports.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import subprocess
import sys
import zipfile
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any
from xml.etree import ElementTree as ET

from musx_semantics import decode_score_dat, local_name


FONT_TYPE_COUNT = 45
EFFECT_BITS = {
    "bold": 0x01,
    "italic": 0x02,
    "underline": 0x04,
    "strikeout": 0x20,
    "absolute": 0x40,
    "hidden": 0x80,
}


def child(parent: ET.Element | None, name: str) -> ET.Element | None:
    if parent is None:
        return None
    return next((element for element in parent if local_name(element.tag) == name), None)


def child_text(parent: ET.Element | None, name: str) -> str:
    element = child(parent, name)
    return (element.text or "") if element is not None else ""


def integer(parent: ET.Element | None, name: str) -> int:
    value = child_text(parent, name)
    return int(value) if value else 0


def normalize_font_name(name: str) -> str:
    """Mirror musx::dom::normalizeFontName for private comparison output."""
    result = bytearray()
    for value in name.encode("utf-8"):
        if value < 0x80:
            if value in b" \t\n\v\f\r":
                continue
            if ord("A") <= value <= ord("Z"):
                value += ord("a") - ord("A")
        result.append(value)
    return result.decode("utf-8")


def font_signature(font: dict[str, Any]) -> tuple[str, int, int] | None:
    if font.get("font_status") == "default":
        name = "@default"
    elif font.get("font_status") == "resolved":
        name = str(font.get("normalized_font_name", ""))
    else:
        return None
    if not name:
        return None
    return name, int(font["font_size"]), int(font["effects"])


def parse_companion(path: Path) -> dict[str, Any]:
    try:
        with zipfile.ZipFile(path) as archive:
            root = ET.fromstring(decode_score_dat(archive.read("score.dat")))
        root_children = {local_name(element.tag): element for element in root}
        definitions: dict[int, dict[str, Any]] = {}
        others = root_children.get("others")
        if others is not None:
            for element in others:
                if local_name(element.tag) != "fontName":
                    continue
                font_id = int(element.attrib.get("cmper", "0"))
                name = child_text(element, "name")
                definitions[font_id] = {
                    "font_name": name,
                    "normalized_font_name": normalize_font_name(name),
                    "charset_bank": child_text(element, "charsetBank"),
                    "charset_value": integer(element, "charsetVal"),
                }

        fonts: list[dict[str, Any]] = []
        font_options = child(root_children.get("options"), "fontOptions")
        if font_options is not None:
            for ordinal, element in enumerate(font_options):
                if local_name(element.tag) != "font":
                    continue
                font_id = integer(element, "fontID")
                effects = 0
                effect_element = child(element, "efx")
                if effect_element is not None:
                    for effect in effect_element:
                        effects |= EFFECT_BITS.get(local_name(effect.tag), 0)
                definition = definitions.get(font_id, {})
                fonts.append({
                    "ordinal": ordinal,
                    "font_type": element.attrib.get("type", ""),
                    "font_id": font_id,
                    "font_size": integer(element, "fontSize"),
                    "effects": effects,
                    "font_status": "default" if font_id == 0 else (
                        "resolved" if definition else "missing"),
                    "font_name": definition.get("font_name", ""),
                    "normalized_font_name": definition.get("normalized_font_name", ""),
                    "charset_bank": definition.get("charset_bank", ""),
                    "charset_value": definition.get("charset_value", ""),
                })
        return {"status": "ok", "fonts": fonts, "font_definitions": len(definitions)}
    except Exception as error:  # retain private corpus failures for investigation
        return {"status": "error", "error": f"{type(error).__name__}: {error}", "fonts": []}


def write_csv(path: Path, rows: list[dict[str, Any]], fields: list[str]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows(rows)


def load_observations(path: Path) -> tuple[
        dict[str, dict[str, Any]], dict[str, list[dict[str, Any]]],
        dict[str, list[dict[str, Any]]]]:
    summaries: dict[str, dict[str, Any]] = {}
    tuples: dict[str, list[dict[str, Any]]] = defaultdict(list)
    definitions: dict[str, list[dict[str, Any]]] = defaultdict(list)
    with path.open(encoding="utf-8") as handle:
        for line in handle:
            observation = json.loads(line)
            corpus_id = observation["corpus_id"]
            if observation["kind"] == "summary":
                summaries[corpus_id] = observation
            elif observation["kind"] == "tuple":
                tuples[corpus_id].append(observation)
            elif observation["kind"] == "font_definition":
                definitions[corpus_id].append(observation)
    for values in tuples.values():
        values.sort(key=lambda value: int(value["ordinal"]))
    return summaries, tuples, definitions


def counter_dict(counter: Counter[Any]) -> dict[str, int]:
    return {str(key): value for key, value in sorted(counter.items(), key=lambda item: str(item[0]))}


def nested_counter_dict(counters: dict[str, Counter[Any]]) -> dict[str, dict[str, int]]:
    return {key: counter_dict(value) for key, value in sorted(counters.items())}


def increment_summary_counters(
        summary: dict[str, Any], counters: dict[str, Counter[Any]], weight: int = 1) -> None:
    epoch = str(summary.get("epoch", "unknown"))
    for field in (
        "font_option_count", "complete_font_option_field_count",
        "recovered_font_option_count", "default_font_option_count",
        "font_definition_count", "source_font_definition_count",
        "cloned_font_definition_count", "dangling_nonzero_font_option_count",
        "duplicate_nonzero_font_name_count",
        "introduced_duplicate_nonzero_font_name_count", "warning_count",
    ):
        counters[field][summary.get(field, "missing")] += weight
    counters["font_option_count_by_epoch"][
        f"{epoch}:{summary.get('font_option_count', 'missing')}"] += weight


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("inventory_csv", type=Path)
    parser.add_argument("--survey-id", required=True)
    parser.add_argument("--analysis-id", default="font_options_all")
    parser.add_argument("--source-probe", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--cohort", choices=("all", "fin27"), default="all")
    parser.add_argument("--include-fallback-unique", action="store_true")
    args = parser.parse_args()

    pair_qualities = {"adjacent-exact"}
    if args.include_fallback_unique:
        pair_qualities.add("fallback-unique")

    with args.inventory_csv.open(newline="", encoding="utf-8") as handle:
        inventory = list(csv.DictReader(handle))

    args.output_dir.mkdir(parents=True, exist_ok=True)
    selection: list[dict[str, Any]] = []
    excluded = Counter()
    for item in inventory:
        source = Path(item["source_path"])
        if not source.is_file():
            excluded["unreadable-source"] += 1
            continue
        quality = item["export_match"]
        export = Path(item["export_path"]) if item["export_path"] else None
        has_companion = quality in pair_qualities and export is not None and export.is_file()
        if args.cohort == "fin27" and not has_companion:
            excluded[f"companion:{quality or 'unclassified'}"] += 1
            continue
        source_hash = item["source_sha256"]
        selection.append({
            "occurrence_id": len(selection) + 1,
            "survey_id": args.survey_id,
            "corpus_id": "mus-" + source_hash[:16],
            "source_hash": source_hash,
            "source_origin": item["origin"],
            "source_name": item["member_path"] or item["source_relative"] or source.name,
            "selector": args.cohort,
            "source_path": str(source),
            "fin27_available": has_companion,
            "fin27_path": str(export) if has_companion else "",
            "fin27_hash": item["export_sha256"] if has_companion else "",
            "fin27_pair_quality": quality if has_companion else "",
            "saving_product": item["saving_product"],
        })

    selection_fields = [
        "occurrence_id", "survey_id", "corpus_id", "source_hash", "source_origin",
        "source_name", "selector", "source_path", "fin27_available", "fin27_path", "fin27_hash",
        "fin27_pair_quality", "saving_product",
    ]
    write_csv(args.output_dir / "selection.csv", selection, selection_fields)

    unique_sources: dict[str, str] = {}
    for item in selection:
        unique_sources.setdefault(str(item["corpus_id"]), str(item["source_path"]))
    source_input = args.output_dir / "source_input.tsv"
    with source_input.open("w", encoding="utf-8") as handle:
        for corpus_id, source_path in sorted(unique_sources.items()):
            if "\t" in source_path or "\n" in source_path:
                raise ValueError("source path contains a tab or newline")
            handle.write(f"{corpus_id}\t{source_path}\n")

    source_output = args.output_dir / "source_observations.jsonl"
    subprocess.run([args.source_probe, source_input, source_output], check=True)
    source_summaries, source_tuples, _source_definitions = load_observations(source_output)
    for item in selection:
        summary = source_summaries.get(
            str(item["corpus_id"]), {"status": "missing", "error": "probe emitted no result"})
        if summary["status"] != "ok":
            print(f"IMPORT FAILURE: {item['source_name']}", file=sys.stderr)
            print(f"  private path: {item['source_path']}", file=sys.stderr)
            print(f"  error: {summary.get('error', summary['status'])}", file=sys.stderr)

    companion_cache: dict[str, dict[str, Any]] = {}
    companion_rows: list[dict[str, Any]] = []
    comparison_rows: list[dict[str, Any]] = []
    for item in selection:
        if not item["fin27_available"]:
            continue
        export_key = str(item["fin27_hash"] or item["fin27_path"])
        if export_key not in companion_cache:
            companion_cache[export_key] = parse_companion(Path(str(item["fin27_path"])))
        companion = companion_cache[export_key]
        corpus_id = str(item["corpus_id"])
        summary = source_summaries.get(corpus_id, {"status": "missing"})
        if companion["status"] != "ok":
            continue
        for font in companion["fonts"]:
            companion_rows.append({
                "occurrence_id": item["occurrence_id"], "corpus_id": corpus_id,
                "pair_quality": item["fin27_pair_quality"], **font,
            })
        if summary.get("status") != "ok":
            continue
        output_by_ordinal = {
            int(value["ordinal"]): value for value in source_tuples.get(corpus_id, [])
        }
        companion_by_ordinal = {
            int(value["ordinal"]): value for value in companion["fonts"]
        }
        for ordinal in range(FONT_TYPE_COUNT):
            output_font = output_by_ordinal.get(ordinal)
            companion_font = companion_by_ordinal.get(ordinal)
            if output_font is None:
                outcome = "missing-output-ordinal"
                origin = "missing"
            elif companion_font is None:
                outcome = "missing-companion-ordinal"
                origin = str(output_font.get("font_id_origin", "unknown"))
            else:
                origins = {
                    str(output_font.get(f"{field}_origin", "unknown"))
                    for field in ("font_id", "font_size", "effects")
                }
                origin = next(iter(origins)) if len(origins) == 1 else "mixed"
                output_signature = font_signature(output_font)
                companion_signature = font_signature(companion_font)
                if output_signature is None:
                    outcome = "unresolved-output-font"
                elif companion_signature is None:
                    outcome = "unresolved-companion-font"
                elif output_signature == companion_signature:
                    outcome = f"{origin}-match"
                else:
                    outcome = f"{origin}-variance"
            comparison_rows.append({
                "occurrence_id": item["occurrence_id"], "corpus_id": corpus_id,
                "pair_quality": item["fin27_pair_quality"],
                "saving_product": item["saving_product"],
                "epoch": summary.get("epoch", ""),
                "source_version": summary.get("source_version", ""),
                "ordinal": ordinal,
                "font_type": companion_font.get("font_type", "") if companion_font else "",
                "origin": origin, "outcome": outcome,
                "output_font_id": output_font.get("font_id", "") if output_font else "",
                "output_font_name": output_font.get("font_name", "") if output_font else "",
                "output_normalized_font_name": output_font.get(
                    "normalized_font_name", "") if output_font else "",
                "output_font_size": output_font.get("font_size", "") if output_font else "",
                "output_effects": output_font.get("effects", "") if output_font else "",
                "companion_font_id": companion_font.get("font_id", "") if companion_font else "",
                "companion_font_name": companion_font.get("font_name", "") if companion_font else "",
                "companion_normalized_font_name": companion_font.get(
                    "normalized_font_name", "") if companion_font else "",
                "companion_font_size": companion_font.get("font_size", "") if companion_font else "",
                "companion_effects": companion_font.get("effects", "") if companion_font else "",
            })

    write_csv(args.output_dir / "companion_font_options.csv", companion_rows, [
        "occurrence_id", "corpus_id", "pair_quality", "ordinal", "font_type",
        "font_id", "font_size", "effects", "font_status", "font_name",
        "normalized_font_name", "charset_bank", "charset_value",
    ])
    write_csv(args.output_dir / "semantic_comparisons.csv", comparison_rows, [
        "occurrence_id", "corpus_id", "pair_quality", "saving_product", "epoch",
        "source_version", "ordinal", "font_type", "origin", "outcome",
        "output_font_id", "output_font_name", "output_normalized_font_name",
        "output_font_size", "output_effects", "companion_font_id", "companion_font_name",
        "companion_normalized_font_name", "companion_font_size", "companion_effects",
    ])

    occurrence_status = Counter()
    distinct_status = Counter()
    distinct_counters: dict[str, Counter[Any]] = defaultdict(Counter)
    occurrence_counters: dict[str, Counter[Any]] = defaultdict(Counter)
    occurrences_by_source = Counter(str(item["corpus_id"]) for item in selection)
    for corpus_id in unique_sources:
        summary = source_summaries.get(corpus_id, {"status": "missing"})
        distinct_status[summary["status"]] += 1
        occurrence_status[summary["status"]] += occurrences_by_source[corpus_id]
        if summary["status"] == "ok":
            increment_summary_counters(summary, distinct_counters)
            increment_summary_counters(
                summary, occurrence_counters, occurrences_by_source[corpus_id])

    unique_companions = {
        str(item["fin27_hash"] or item["fin27_path"]): companion_cache[
            str(item["fin27_hash"] or item["fin27_path"])]
        for item in selection if item["fin27_available"]
    }
    companion_counts = Counter(
        len(companion["fonts"]) if companion["status"] == "ok" else "error"
        for companion in unique_companions.values())
    companion_sequences = Counter(
        tuple(font["font_type"] for font in companion["fonts"])
        for companion in unique_companions.values() if companion["status"] == "ok")

    distinct_comparisons: dict[tuple[str, str, int], dict[str, Any]] = {}
    for row in comparison_rows:
        item = selection[int(row["occurrence_id"]) - 1]
        key = (str(row["corpus_id"]), str(item["fin27_hash"]), int(row["ordinal"]))
        distinct_comparisons.setdefault(key, row)
    comparison_occurrence = Counter(row["outcome"] for row in comparison_rows)
    comparison_distinct = Counter(row["outcome"] for row in distinct_comparisons.values())
    comparison_by_epoch: dict[str, Counter[Any]] = defaultdict(Counter)
    comparison_by_type: dict[str, Counter[Any]] = defaultdict(Counter)
    for row in distinct_comparisons.values():
        comparison_by_epoch[str(row["epoch"])][row["outcome"]] += 1
        comparison_by_type[str(row["font_type"])][row["outcome"]] += 1

    aggregate = {
        "analysis_id": args.analysis_id,
        "selection": {
            "inventory_occurrences": len(inventory),
            "selected_occurrences": len(selection),
            "distinct_sources": len(unique_sources),
            "occurrences_with_companions": sum(
                bool(item["fin27_available"]) for item in selection),
            "distinct_companions": len(unique_companions),
            "by_pair_quality_occurrences": counter_dict(Counter(
                item["fin27_pair_quality"] for item in selection
                if item["fin27_available"])),
            "excluded_inventory_rows": counter_dict(excluded),
        },
        "imports": {
            "status_by_distinct_source": counter_dict(distinct_status),
            "status_by_occurrence": counter_dict(occurrence_status),
            "successful_distinct_source_distributions": {
                key: counter_dict(value) for key, value in sorted(distinct_counters.items())
            },
            "successful_occurrence_distributions": {
                key: counter_dict(value) for key, value in sorted(occurrence_counters.items())
            },
        },
        "companions": {
            "font_option_counts_by_distinct_companion": counter_dict(companion_counts),
            "distinct_type_sequences": [
                {"count": count, "types": list(sequence)}
                for sequence, count in companion_sequences.most_common()
            ],
        },
        "semantic_comparison": {
            "distinct_source_companion_pairs": len({
                (key[0], key[1]) for key in distinct_comparisons}),
            "outcomes_by_distinct_ordinal": counter_dict(comparison_distinct),
            "outcomes_by_occurrence_ordinal": counter_dict(comparison_occurrence),
            "outcomes_by_epoch_distinct_ordinal": nested_counter_dict(comparison_by_epoch),
            "outcomes_by_type_distinct_ordinal": nested_counter_dict(comparison_by_type),
        },
        "method_hash": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
    }
    (args.output_dir / "aggregate.json").write_text(
        json.dumps(aggregate, indent=2, ensure_ascii=False, sort_keys=True) + "\n",
        encoding="utf-8")


if __name__ == "__main__":
    main()
