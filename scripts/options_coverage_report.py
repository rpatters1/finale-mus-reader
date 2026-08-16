#!/usr/bin/env python3
# Copyright (c) 2026 Robert G. Patterson
# SPDX-License-Identifier: MIT

"""Aggregate options_coverage_probe observations into an epoch coverage report.

Reads the private JSON Lines emitted by tools/options_coverage_probe, optionally
compares each observation with an independently extracted Finale 27 companion, and
applies the acceptance rule from implement-musxdom-class: no epoch present in the
cohort may be left entirely uncovered for any class under study.

All inputs are explicit paths. Output is aggregate and carries no source path, so it
may be pasted into tracked findings; per-document detail stays in the private inputs.
Exits nonzero when the acceptance rule fails.
"""

import argparse
import collections
import json
import os
import re
import sys
import zipfile

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__))))
from musx_semantics import decode_score_dat  # noqa: E402

EPOCH_ORDER = ["coda-banner", "uncompressed", "dcl", "zlib", "unknown"]

# The classes under study. Each names the observation key that carries its recovered
# values and a label for the report.
CLASSES = [
    ("clef_options", "ClefOptions"),
    ("font_options", "FontOptions"),
    ("mmrest_options", "MultimeasureRestOptions"),
    ("font_definitions", "FontDefinition"),
]

# MultimeasureRestOptions: probe key, companion element, musxdom member. The element names
# are musxdom's own XML mapping and are not the member names -- measWidth is <meaSpace>,
# numAdjY is <numdec>, useSymsThreshold is <threshold>, symSpacing is <spacing> and
# useSymbols is <useCharRestStyle>. An absent element means the field is 0 or false.
MMREST_NUMERIC = [
    ("meas_width", "meaSpace", "measWidth"),
    ("num_adj_y", "numdec", "numAdjY"),
    ("shape_def", "shapeDef", "shapeDef"),
    ("num_start", "numStart", "numStart"),
    ("use_syms_threshold", "threshold", "useSymsThreshold"),
    ("sym_spacing", "spacing", "symSpacing"),
    ("num_adj_x", "numAdjX", "numAdjX"),
    ("start_adjust", "startAdjust", "startAdjust"),
    ("end_adjust", "endAdjust", "endAdjust"),
]
MMREST_BOOLEAN = [
    ("use_symbols", "useCharRestStyle", "useSymbols"),
    ("no_horizontal_stretch", "noHorizontalStretch", "noHorizontalStretch"),
    ("auto_update_mm_rests", "autoUpdateMmRests", "autoUpdateMmRests"),
]

FONT_TYPE_NAMES = {}  # ordinal -> companion XML type name, filled from the baseline


def load_observations(paths):
    rows = []
    for path in paths:
        with open(path, encoding="utf-8") as handle:
            for line in handle:
                line = line.strip()
                if line:
                    rows.append(json.loads(line))
    return rows


def recovered_origins(values):
    """Origins that represent a source-derived value rather than a seeded default."""
    return sum(1 for v in values if v in ("legacy-mus", "legacy-behavior"))


def clef_coverage(obs):
    """Per-document ClefOptions coverage counts."""
    clef = obs.get("clef_options")
    if not clef:
        return None
    scalars = [v for k, v in clef.items() if k.startswith("origin_")]
    defs = clef.get("clef_defs", [])
    def_origins = []
    for d in defs:
        def_origins.extend(v for k, v in d.items() if k.startswith("origin_"))
    return {
        "def_count": len(defs),
        "scalar_recovered": recovered_origins(scalars),
        "scalar_total": len(scalars),
        "def_recovered": recovered_origins(def_origins),
        "def_total": len(def_origins),
        "dangling_shapes": sum(1 for d in defs if d.get("dangling_shape")),
        "any_recovered": recovered_origins(scalars) + recovered_origins(def_origins) > 0,
    }


def font_option_coverage(obs):
    fonts = obs.get("font_options")
    if fonts is None:
        return None
    origins = [f.get("origin", "absent") for f in fonts]
    return {
        "count": len(fonts),
        "recovered": recovered_origins(origins),
        "dangling": sum(1 for f in fonts if f.get("dangling")),
        "any_recovered": recovered_origins(origins) > 0,
    }


# Fields the reader asserts for every document regardless of what it read. They carry no
# information about coverage -- counting them would make every epoch pass the acceptance rule
# by construction, including one where nothing was decoded at all -- so they are excluded from
# the coverage counts. They are still compared against companions like any other field.
UNCONDITIONAL_MMREST_FIELDS = {"origin_noHorizontalStretch"}


def mmrest_coverage(obs):
    mmrest = obs.get("mmrest_options")
    if not mmrest:
        return None
    origins = [v for k, v in mmrest.items()
               if k.startswith("origin_") and k not in UNCONDITIONAL_MMREST_FIELDS]
    return {
        "recovered": recovered_origins(origins),
        "total": len(origins),
        "dangling_shapes": 1 if mmrest.get("dangling_shape") else 0,
        "any_recovered": recovered_origins(origins) > 0,
    }


def font_definition_coverage(obs):
    defs = obs.get("font_definitions")
    if defs is None:
        return None
    origins = [d.get("origin", "absent") for d in defs]
    return {
        "count": len(defs),
        "recovered": recovered_origins(origins),
        "any_recovered": recovered_origins(origins) > 0,
    }


def normalize_font(name):
    """Mirror musxdom's normalizeFontName. Comparison only; spellings are kept."""
    return re.sub(r"[^a-z0-9]", "", (name or "").lower())


def read_companion(path):
    """Extract the companion's own view of the three classes, independently."""
    xml = decode_score_dat(zipfile.ZipFile(path).read("score.dat")).decode("utf-8", "replace")
    names = {}
    for m in re.finditer(r'<fontName cmper="(\d+)">(.*?)</fontName>', xml, re.S):
        n = re.search(r"<name>(.*?)</name>", m.group(2))
        names[m.group(1)] = n.group(1) if n else ""
    fonts = {}
    fo = re.search(r"<fontOptions>(.*?)</fontOptions>", xml, re.S)
    if fo:
        for m in re.finditer(r'<font type="(\w+)">(.*?)</font>', fo.group(1), re.S):
            fid = re.search(r"<fontID>(\d+)</fontID>", m.group(2))
            size = re.search(r"<fontSize>(\d+)</fontSize>", m.group(2))
            fonts[m.group(1)] = (
                normalize_font(names.get(fid.group(1) if fid else "0", "")),
                int(size.group(1)) if size else 0,
            )
    clefs = []
    co = re.search(r"<clefOptions>(.*?)</clefOptions>", xml, re.S)
    scalars = {}
    if co:
        body = co.group(1)
        # Element names come from musxdom's own XML mapping, not from the member names:
        # clefKeySepar is written <clefKey> and clefTimeSepar is written <clefTime>.
        # Guessing them produced 319 false disagreements before this was checked.
        # An absent element means the field keeps its default, which for these is 0.
        for tag in ("defaultClef", "endMeasClefPercent", "endMeasClefPosAdd",
                    "clefFront", "clefBack", "clefKey", "clefTime"):
            m = re.search(r"<%s>(-?\d+)</%s>" % (tag, tag), body)
            scalars[tag] = int(m.group(1)) if m else 0
        for tag in ("showClefFirstSystemOnly", "cautionaryClefChanges"):
            scalars[tag] = bool(re.search(r"<%s/>|<%s>1</%s>" % (tag, tag, tag), body))
        for m in re.finditer(r"<clefDef index=\"(\d+)\">(.*?)</clefDef>", body, re.S):
            d = m.group(2)

            def num(tag, default=0):
                mm = re.search(r"<%s>(-?\d+)</%s>" % (tag, tag), d)
                return int(mm.group(1)) if mm else default

            clefs.append({
                "index": int(m.group(1)),
                "middle_c_pos": num("adjust"),
                "clef_char": num("clefChar"),
                "staff_position": num("clefYDisp"),
                "baseline_adjust": num("baseAdjust"),
                "shape_id": num("shapeID"),
                "is_shape": "<isShape/>" in d,
            })
    mmrest = {}
    mo = re.search(r"<multimeasureRestOptions>(.*?)</multimeasureRestOptions>", xml, re.S)
    if mo:
        body = mo.group(1)
        for _, tag, _ in MMREST_NUMERIC:
            m = re.search(r"<%s>(-?\d+)</%s>" % (tag, tag), body)
            mmrest[tag] = int(m.group(1)) if m else 0
        for _, tag, _ in MMREST_BOOLEAN:
            mmrest[tag] = bool(re.search(r"<%s/>|<%s>1</%s>" % (tag, tag, tag), body))
    return {"fonts": fonts,
            "font_names": {normalize_font(v) for v in names.values() if v},
            "clefs": clefs, "clef_scalars": scalars, "mmrest": mmrest}


def compare_companion(obs, comp):
    """Classify each comparable value. Returns Counter of (class, field, outcome)."""
    out = collections.Counter()
    fonts = obs.get("font_options") or []
    ours_by_ordinal = {f["ordinal"]: f for f in fonts}
    for ordinal, type_name in FONT_TYPE_NAMES.items():
        theirs = comp["fonts"].get(type_name)
        ours = ours_by_ordinal.get(ordinal)
        if theirs is None or ours is None:
            continue
        # Compare normalized faces: "EngraverTextT" and "Engraver Text T" are the
        # same font, and comparing raw spellings invents disagreements.
        same_face = (ours.get("normalized_font_name")
                     or normalize_font(ours.get("font_name"))) == (theirs[0] or "")
        same_size = ours["font_size"] == theirs[1]
        if same_face and same_size:
            outcome = "preserved"
        elif same_face:
            outcome = "size-differs"
        elif same_size:
            outcome = "face-differs"
        else:
            outcome = "both-differ"
        out[("FontOptions", ours.get("origin", "absent"), outcome)] += 1

    clef = obs.get("clef_options")
    if clef and comp["clefs"]:
        theirs = {c["index"]: c for c in comp["clefs"]}
        for d in clef.get("clef_defs", []):
            t = theirs.get(d["index"])
            if t is None:
                out[("ClefOptions", "def", "absent-in-companion")] += 1
                continue
            for field in ("middle_c_pos", "clef_char", "staff_position", "baseline_adjust"):
                origin = d.get("origin_" + {
                    "middle_c_pos": "middleCPos", "clef_char": "clefChar",
                    "staff_position": "staffPosition",
                    "baseline_adjust": "baselineAdjust"}[field], "absent")
                outcome = "preserved" if d[field] == t[field] else "differs"
                out[("ClefOptions." + field, origin, outcome)] += 1
        for tag, key, member in (
                ("defaultClef", "default_clef", "defaultClef"),
                ("endMeasClefPercent", "clef_change_percent", "clefChangePercent"),
                ("endMeasClefPosAdd", "clef_change_offset", "clefChangeOffset"),
                ("clefFront", "clef_front_separ", "clefFrontSepar"),
                ("clefBack", "clef_back_separ", "clefBackSepar"),
                ("clefKey", "clef_key_separ", "clefKeySepar"),
                ("clefTime", "clef_time_separ", "clefTimeSepar"),
                ("showClefFirstSystemOnly", "show_clef_first_system_only",
                 "showClefFirstSystemOnly"),
                ("cautionaryClefChanges", "cautionary_clef_changes",
                 "cautionaryClefChanges")):
            outcome = "preserved" if clef[key] == comp["clef_scalars"][tag] else "differs"
            out[("ClefOptions." + tag, clef.get("origin_" + member, "absent"), outcome)] += 1

    mmrest = obs.get("mmrest_options")
    if mmrest and comp["mmrest"]:
        for key, tag, member in MMREST_NUMERIC + MMREST_BOOLEAN:
            outcome = "preserved" if mmrest[key] == comp["mmrest"][tag] else "differs"
            out[("MultimeasureRestOptions." + tag,
                 mmrest.get("origin_" + member, "absent"), outcome)] += 1

    ours_names = {normalize_font(d["name"]) for d in (obs.get("font_definitions") or [])
                  if d.get("name")}
    missing = comp["font_names"] - ours_names
    out[("FontDefinition", "pool", "companion-face-present")] += len(
        comp["font_names"] & ours_names)
    out[("FontDefinition", "pool", "companion-face-missing")] += len(missing)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--observations", nargs="+", required=True)
    ap.add_argument("--companions", nargs="*", default=[],
                    help="companions.json files mapping corpus_id -> [path, pair_quality]")
    ap.add_argument("--occurrences", nargs="*", default=[])
    ap.add_argument("--font-type-map", help="musxdom FieldPopulatorsOptions.cpp, whose "
                    "FontType enum mapping gives ordinal order authoritatively")
    ap.add_argument("--pair-quality", default="adjacent-exact")
    args = ap.parse_args()

    if args.font_type_map:
        # Declaration order in the enum mapping IS the ordinal. Deriving it from a
        # document's element order would silently break if any document omitted a type.
        text = open(args.font_type_map, encoding="utf-8", errors="replace").read()
        block = re.search(r"MUSX_XML_ENUM_MAPPING\(FontOptions::FontType,\s*\{(.*?)\}\);",
                          text, re.S)
        if not block:
            sys.exit("could not find the FontType enum mapping")
        for i, m in enumerate(re.finditer(r'\{"(\w+)",\s*FontOptions::FontType::',
                                          block.group(1))):
            FONT_TYPE_NAMES[i] = m.group(1)

    rows = load_observations(args.observations)
    occ = collections.Counter()
    for p in args.occurrences:
        occ.update(json.load(open(p)))
    companions = {}
    for p in args.companions:
        companions.update(json.load(open(p)))

    by_epoch = collections.defaultdict(lambda: collections.defaultdict(int))
    failures = []
    for obs in rows:
        if obs.get("status") != "ok":
            failures.append(obs)
            by_epoch["(failed import)"]["docs"] += 1
            continue
        e = by_epoch[obs.get("epoch", "unknown")]
        e["docs"] += 1
        e["occurrences"] += occ.get(obs["corpus_id"], 1)
        e["warnings"] += obs.get("warning_count", 0)
        c = clef_coverage(obs)
        if c:
            e["clef_docs"] += 1
            e["clef_any"] += 1 if c["any_recovered"] else 0
            e["clef_defs"] += c["def_count"]
            e["clef_def_recovered"] += c["def_recovered"]
            e["clef_def_total"] += c["def_total"]
            e["clef_scalar_recovered"] += c["scalar_recovered"]
            e["clef_scalar_total"] += c["scalar_total"]
            e["clef_dangling_shapes"] += c["dangling_shapes"]
        f = font_option_coverage(obs)
        if f:
            e["fo_docs"] += 1
            e["fo_any"] += 1 if f["any_recovered"] else 0
            e["fo_recovered"] += f["recovered"]
            e["fo_total"] += f["count"]
            e["fo_dangling"] += f["dangling"]
        m = mmrest_coverage(obs)
        if m:
            e["mm_docs"] += 1
            e["mm_any"] += 1 if m["any_recovered"] else 0
            e["mm_recovered"] += m["recovered"]
            e["mm_total"] += m["total"]
            e["mm_dangling_shapes"] += m["dangling_shapes"]
        d = font_definition_coverage(obs)
        if d:
            e["fd_docs"] += 1
            e["fd_any"] += 1 if d["any_recovered"] else 0
            e["fd_recovered"] += d["recovered"]
            e["fd_total"] += d["count"]

    print("=" * 78)
    print("SELECTION")
    print("=" * 78)
    total_docs = sum(v["docs"] for v in by_epoch.values())
    total_occ = sum(v.get("occurrences", 0) for v in by_epoch.values())
    print(f"  distinct corpus_id observed : {total_docs}")
    print(f"  source occurrences          : {total_occ}")
    print(f"  import failures             : {len(failures)}")
    print(f"  companions available        : {len(companions)}")

    print()
    print("=" * 78)
    print("READER COVERAGE BY EPOCH  (recovered = legacy-mus or legacy-behavior)")
    print("=" * 78)
    header = (f"{'epoch':<14}{'docs':>7}{'occurs':>8}  "
              f"{'ClefOptions':>22}  {'FontOptions':>20}  {'MmRestOptions':>20}  "
              f"{'FontDefinition':>20}")
    print(header)
    for epoch in EPOCH_ORDER + sorted(k for k in by_epoch if k not in EPOCH_ORDER):
        if epoch not in by_epoch:
            continue
        e = by_epoch[epoch]
        if epoch == "(failed import)":
            print(f"{epoch:<14}{e['docs']:>7}")
            continue
        clef = (f"{e['clef_any']}/{e['clef_docs']} docs "
                f"{e['clef_def_recovered'] + e['clef_scalar_recovered']}f")
        fo = f"{e['fo_any']}/{e['fo_docs']} docs {e['fo_recovered']}f"
        mm = f"{e['mm_any']}/{e['mm_docs']} docs {e['mm_recovered']}f"
        fd = f"{e['fd_any']}/{e['fd_docs']} docs {e['fd_recovered']}f"
        print(f"{epoch:<14}{e['docs']:>7}{e.get('occurrences', 0):>8}  "
              f"{clef:>22}  {fo:>20}  {mm:>20}  {fd:>20}")
    print()
    print("  'x/y docs' = documents with at least one source-derived value / documents")
    print("  'Nf'       = total source-derived fields recovered across the epoch")

    print()
    print("=" * 78)
    print("ACCEPTANCE — every epoch present must have coverage for every class")
    print("=" * 78)
    ok = True
    for epoch in EPOCH_ORDER:
        if epoch not in by_epoch or epoch == "unknown":
            continue
        e = by_epoch[epoch]
        for key, label in (("clef_any", "ClefOptions"), ("fo_any", "FontOptions"),
                           ("mm_any", "MultimeasureRestOptions"),
                           ("fd_any", "FontDefinition")):
            if e.get(key, 0) == 0:
                print(f"  FAIL  {epoch:<14} {label}: no document recovered any value")
                ok = False
            else:
                pct = 100.0 * e[key] / max(1, e["docs"])
                print(f"  pass  {epoch:<14} {label}: {e[key]}/{e['docs']} docs ({pct:.1f}%)")
    # A corpus discovered by content sniffing contains files that are not Finale
    # documents at all. Refusing one is correct behavior, not a coverage failure; only
    # a file the reader accepted as legacy MUS and then could not import is a defect.
    rejected = [f for f in failures
                if "not a recognized legacy Finale MUS file" in f.get("error", "")]
    broke = [f for f in failures if f not in rejected]
    if rejected:
        print(f"  note  {len(rejected)} file(s) rejected as not legacy MUS "
              f"(correct for a content-sniffed corpus)")
    if broke:
        print(f"  FAIL  {len(broke)} document(s) were accepted as MUS but failed to import")
        ok = False

    quality_ok = 0
    if companions and FONT_TYPE_NAMES:
        print()
        print("=" * 78)
        print(f"COMPANION COMPARISON  (pair quality: {args.pair_quality})")
        print("=" * 78)
        outcomes = collections.Counter()
        by_ep = collections.defaultdict(collections.Counter)
        seen = 0
        for obs in rows:
            if obs.get("status") != "ok":
                continue
            entry = companions.get(obs["corpus_id"])
            if not entry:
                continue
            path, quality = entry
            if quality != args.pair_quality:
                continue
            try:
                comp = read_companion(path)
            except Exception as error:  # noqa: BLE001
                print(f"  companion unreadable for {obs['corpus_id']}: {error}")
                continue
            seen += 1
            got = compare_companion(obs, comp)
            outcomes.update(got)
            for k, v in got.items():
                by_ep[obs["epoch"]][(k[0], k[2])] += v
        quality_ok = seen
        print(f"  compared {seen} document(s)\n")
        print(f"  {'field':<42}{'origin':<18}{'outcome':<24}{'count':>8}")
        for (field, origin, outcome), n in sorted(
                outcomes.items(), key=lambda kv: (-kv[1], kv[0])):
            print(f"  {field:<42}{origin:<18}{outcome:<24}{n:>8}")

    print()
    print("RESULT:", "ACCEPTED" if ok else "NOT ACCEPTED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
