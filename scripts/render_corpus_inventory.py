#!/usr/bin/env python3
"""Join binary and MUSX analyses into the human-readable corpus inventory."""

from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path


def era(product: str) -> tuple[str, str, str]:
    if product == "unknown":
        return "unclassified", "low", "uncertain"
    if product in {"1.0.0", "1.8.7", "2.0.1", "2.6"}:
        # Coda-banner era. The version is explicit in the banner, which is the
        # only place it appears, so confidence is high even though the era's
        # directory spans are unresolved. 1.0.0 spells the banner a third way,
        # `Finale` + MacRoman trademark sign + version + `ENIGA Structures`, and
        # its 22 specimens probe at median body entropy 2.80 with no compressed
        # member, which places it with this group rather than apart from it.
        return "Coda banner", "high", "uncertain"
    if product in {"3.0", "3.2", "3.5", "3.7", "3.8", "97", "98", "99", "2000"}:
        # 3.8, 98 and 99 are placed here on observation, not on their position in
        # the version sequence: median body entropy 3.54, 3.68 and 2.21, with no
        # zlib member and no validated wrapper in any specimen, matching the rest
        # of this group and nothing like the ~7.65 of the DCL-compressed era.
        return "low-entropy legacy", "high", "likely"
    if product in {"2001", "2002", "2003", "2004", "2004b", "2005"}:
        return "high-entropy legacy", "high", "likely"
    if product == "2006":
        return "high-entropy legacy", "high", "unlikely/unknown"
    if product in {"2007", "2008"}:
        return "typed-zlib transition", "high", "no"
    if product in {"2009", "2010", "2011", "2012"}:
        return "typed-zlib stable", "high", "no"
    # Anything unlisted is unclassified, not assumed to be the newest era. A
    # catch-all that names an era will confidently mislabel the next product
    # string the corpus turns up, which is exactly what happened when the
    # Coda-banner versions stopped reading as "unknown".
    return "unclassified", "low", "uncertain"


def corpus_id(sha256: str) -> str:
    """Stable public identifier; it reveals equality, not the source location."""
    return "mus-" + sha256[:16]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("inventory_csv", type=Path)
    parser.add_argument("structure_csv", type=Path)
    # Optional: a corpus with no modern re-saves has no semantic half at all,
    # and rendering the binary half alone is better than fabricating an empty
    # semantics file so the argument can stay required.
    parser.add_argument("semantics_csv", type=Path, nargs="?")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    with args.inventory_csv.open(newline="", encoding="utf-8") as handle:
        inventory = list(csv.DictReader(handle))
    with args.structure_csv.open(newline="", encoding="utf-8") as handle:
        structures = {row["source_relative"]: row for row in csv.DictReader(handle)}
    semantics: dict[str, dict[str, str]] = {}
    if args.semantics_csv is not None:
        with args.semantics_csv.open(newline="", encoding="utf-8") as handle:
            semantics = {row["source_relative"]: row for row in csv.DictReader(handle)}

    lines = [
        "# Corpus Inventory",
        "",
        "This table includes every legacy data-fork candidate examined, whether it was found by its `.mus` suffix or, "
        "where the survey enabled content sniffing, by its header alone — classic Mac Finale kept the file type in the "
        "resource fork, so its documents commonly carry no extension. Saving product comes from the file banner, in any "
        "of its three spellings, which are tabulated in "
        "[format/container/header.md](../../format/container/header.md#the-three-banner-spellings). "
        "`unknown` means no banner was recognized in any of them. "
        "SHA-256 hashes cover complete files. "
        "An em dash means no exact adjacent Finale 27 export was found. The public table shows content-derived IDs "
        "only. Filenames and paths are local evidence and are never published, because a filename can name a work, "
        "a client, or a person; resolve an ID through the ignored `private/generated/<survey_id>/corpus_locations.csv` file "
        "described in the README.",
        "",
        "Version confidence is high when the banner is explicit and low for pre-banner path-based classification. "
        "ETF likelihood is an eligibility estimate, not a verified open/export result. `Created app` is the creator tuple "
        "preserved by Finale 27 and helps identify upgraded documents. `Parts` is based on converted `partDef` records; "
        "conversion may expand sharing.",
        "",
        "| # | Corpus ID | Bytes | Source SHA-256 | Save product / era | Created app | ETF | Parts / notable converted features | Export match | Export SHA-256 |",
        "|---:|---|---:|---|---|---|---|---|---|---|",
    ]
    for item in inventory:
        semantic = semantics.get(item["source_relative"], {})
        structure = structures.get(item["source_relative"], {})
        format_era, confidence, etf = era(item["saving_product"])
        created = semantic.get("created_app_version", "") or "—"
        if semantic:
            others = json.loads(semantic["others_types"])
            parts = int(semantic["part_def_count"])
            features = (
                f"parts={parts}; measures={others.get('measSpec', 0)}; staves={others.get('staffSpec', 0)}; "
                f"entries={semantic['entries_count']}; expr={others.get('textExprDef', 0)}; artic={others.get('articDef', 0)}"
            )
        else:
            features = "no exact MUSX reference"
        if structure.get("validated_wrapper_endian", "none") != "none":
            features += f"; wrapper={structure['validated_wrapper_endian']}"
        export_match = item["export_match"] or "—"
        export_hash = f"`{item['export_sha256']}`" if item["export_sha256"] else "—"
        lines.append(
            f"| {item['index']} | `{corpus_id(item['source_sha256'])}` | {item['source_size']} | `{item['source_sha256']}` | "
            f"{item['saving_product']} / {format_era} ({confidence}) | {created} | {etf} | {features} | "
            f"{export_match} | {export_hash} |"
        )
    args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
