#!/usr/bin/env python3
"""Census legacy expression selector combinations against raw companion XML.

Read-only exploratory analysis, not recovery-coverage classification. Same-cmper
pairings are candidates only: IDs may change on upgrade. A matching TextBlock ID
is reported as an additional check, not semantic proof. Unmatched definitions and
missing XML fields stay visible; no XML defaults or numeric enum casts are used.
Inputs are existing corpus_locations.csv and public corpus_manifest.csv files.
The JSON on stdout can contain per-definition observations; keep it private.
"""

from __future__ import annotations

import argparse
import csv
import json
import re
import struct
import subprocess
import sys
import zipfile
from collections import Counter, defaultdict
from pathlib import Path
from xml.etree import ElementTree as ET

from musx_semantics import child, decode_score_dat, local_name, text


ROW = re.compile(r"cmper=(\d+)\s+cmper2=(\d+)\s+inci=(\d+)\s+"
                 r"words=\[([^]]*)\] bytes=([0-9a-f]*)")
AXES = {"x": (6, 9, "horzMeasExprAlign", range(8), range(7)),
        "y": (12, 14, "vertMeasExprAlign", range(4), range(9))}


def records(dump: str) -> dict[int, list[int]]:
    """Reassemble fixed incidences or decode complete variable payloads."""
    header = re.search(r"epoch=(\d+) byteOrder=(big|little)", dump)
    if not header:
        raise ValueError("record_dump omitted epoch/byte order")
    variable = int(header[1]) == 3
    rows = defaultdict(dict)
    for line in dump.splitlines():
        match = ROW.search(line)
        if not match:
            continue
        cmper, second, incidence = map(int, match.group(1, 2, 3))
        if second:
            raise ValueError("unexpected second comparator")
        if incidence in rows[cmper]:
            raise ValueError("duplicate incidence")
        if variable:
            payload = bytes.fromhex(match[5])
            if len(payload) < 36:
                raise ValueError("short variable expression header")
            words = list(struct.unpack((">" if header[2] == "big" else "<")
                                       + "18h", payload[:36]))
        else:
            words = list(map(int, match[4].split()))
        rows[cmper][incidence] = words
    result = {}
    for cmper, incidences in rows.items():
        if sorted(incidences) != list(range(len(incidences))):
            raise ValueError("noncontiguous incidences")
        words = [word for i in sorted(incidences) for word in incidences[i]]
        if len(words) < 18:
            raise ValueError("short fixed expression header")
        result[cmper] = words
    return result


def companion(path: Path) -> dict[int, ET.Element]:
    with zipfile.ZipFile(path) as archive:
        root = ET.fromstring(decode_score_dat(archive.read("score.dat")))
    result = {}
    others = child(root, "others")
    if others is None:
        raise ValueError("companion has no others pool")
    for element in others:
        if local_name(element.tag) != "textExprDef":
            continue
        if int(element.get("part", "0")) != 0:
            continue
        cmper = int(element.attrib["cmper"])
        if cmper in result:
            raise ValueError("duplicate score expression comparator")
        result[cmper] = element
    return result


def combinations(observations: list[dict], axis: str) -> list[dict]:
    mword, nword, _, measures, notes = AXES[axis]
    groups = defaultdict(list)
    for observation in observations:
        words = observation["words"]
        groups[words[mword], words[nword]].append(observation)
    # Include missing combinations, plus any observed out-of-range codes.
    pairs = set(groups) | {(m, n) for m in measures for n in notes}
    result = []
    for measure, note in sorted(pairs):
        samples = groups[measure, note]
        outcomes = Counter(o[axis] for o in samples)
        result.append({"measure": measure, "note": note,
                       "definitions": len(samples),
                       "distinct_sources": len({o["corpus_id"] for o in samples}),
                       "outcomes": dict(sorted(outcomes.items())),
                       "conflict": len(outcomes) > 1,
                       "examples": [{"corpus_id": o["corpus_id"], "cmper": o["cmper"]}
                                    for o in samples[:3]]})
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("locations", type=Path)
    parser.add_argument("manifest", type=Path)
    parser.add_argument("--record-dump", type=Path, required=True)
    args = parser.parse_args()
    with args.manifest.open(newline="", encoding="utf-8") as handle:
        products = {r["corpus_id"]: r["saving_product"] for r in csv.DictReader(handle)}
    with args.locations.open(newline="", encoding="utf-8") as handle:
        locations = list(csv.DictReader(handle))
    counts = Counter(occurrences=len(locations))
    observations, unmatched, failures = [], [], []
    seen = set()
    for item in locations:
        identity = item["corpus_id"]
        if identity in seen:
            counts["duplicate_source_occurrences"] += 1
            continue
        seen.add(identity)
        product = products.get(identity, "")
        year = re.match(r"^(200[4-8])(?:\D|$)", product)
        if not year:
            counts["outside_2004_2008_or_unknown"] += 1
            continue
        counts["selected_sources"] += 1
        if not item["export_path"]:
            counts["missing_companion"] += 1
            continue
        try:
            variable = int(year[1]) >= 2007
            command = [str(args.record_dump.resolve()), item["source_path"],
                       "--class=0x00f1" if variable else "--tag=DT",
                       "--pool=class-other" if variable else "--pool=others"]
            dump = subprocess.run(command, check=True, capture_output=True, text=True).stdout
            source = records(dump)
            target = companion(Path(item["export_path"]))
            counts["sources_with_definitions" if source else "sources_without_definitions"] += 1
            counts["source_definitions"] += len(source)
            counts["companion_definitions"] += len(target)
            for cmper in sorted(source.keys() | target.keys()):
                if cmper not in source or cmper not in target:
                    unmatched.append({"corpus_id": identity, "cmper": cmper,
                                      "side": "companion" if cmper not in source else "source"})
                    continue
                words, element = source[cmper], target[cmper]
                observations.append({"corpus_id": identity, "product": product,
                                     "cmper": cmper, "words": words[:18],
                                     "text_block_id_agrees": str(words[0] & 0xffff) == text(element, "textIDKey"),
                                     **{axis: text(element, spec[2]) or "<omitted>"
                                        for axis, spec in AXES.items()}})
            counts["successful_sources"] += 1
        except Exception as exc:
            failures.append({"corpus_id": identity, "error": str(exc)})
    result = {"pairing": "same-cmper candidates; NOT semantically verified",
              "counts": dict(counts), "failures": failures, "unmatched": unmatched,
              "candidate_pairs": len(observations),
              "text_block_id_disagreements": sum(not o["text_block_id_agrees"] for o in observations),
              "combinations": {axis: combinations(observations, axis) for axis in AXES},
              "observations": observations}
    json.dump(result, sys.stdout, indent=2)
    print()
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
