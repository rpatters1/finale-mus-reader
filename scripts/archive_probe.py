#!/usr/bin/env python3
"""Render the archive-member deliverables from an archive-inclusive inventory.

This no longer opens archives.  ``inventory.py --include-archives`` extracts
candidate members once into a private cache and records them as ordinary rows,
so the whole survey covers them; this step only summarizes the archive-derived
part of that inventory.

Public output carries content-derived IDs only. Archive and member names stay
in the private mapping: a filename can name a work, a client, or a person.
"""

from __future__ import annotations

import argparse
import csv
from pathlib import Path

from archive_sources import specimen_filename
from structure_probe import entropy, parse_zero_trailed_records, wrapper_endian, zlib_members


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("inventory_csv", type=Path, help="corpus_inventory.csv built with --include-archives")
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--summary", type=Path, required=True)
    parser.add_argument("--private-output", type=Path)
    args = parser.parse_args()

    with args.inventory_csv.open(newline="", encoding="utf-8") as handle:
        inventory = list(csv.DictReader(handle))
    members = [row for row in inventory if row.get("origin") == "archive"]

    rows: list[dict[str, object]] = []
    for item in members:
        path = Path(item["source_path"])
        try:
            data = path.read_bytes()
        except OSError:
            continue
        zlib_blocks = zlib_members(data)
        framed = sum(
            len(parsed[3])
            for _, _, decoded in zlib_blocks
            if (parsed := parse_zero_trailed_records(decoded)) is not None
        )
        member_filename = specimen_filename(item)
        body = data[0x200 : min(len(data), 0x4200)]
        rows.append({
            "archive_id": item["archive_id"],
            "member_id": "mus-" + item["source_sha256"][:16],
            "member_size": item["source_size"],
            "member_sha256": item["source_sha256"],
            "is_enigma_binary": item["signature_ok"],
            "saving_product": item["saving_product"],
            "member_name_has_extension": str(bool(Path(member_filename).suffix)),
            "body_prefix": data[0x200:0x210].hex(),
            "body_entropy": f"{entropy(body):.4f}",
            "wrapper_endian": wrapper_endian(data),
            "zlib_members": len(zlib_blocks),
            "framed_records": framed,
            "_archive_path": item["archive_path"],
            "_member_path": item["member_path"],
        })

    fields = ["archive_id", "member_id", "member_size", "member_sha256",
              "is_enigma_binary", "saving_product", "member_name_has_extension", "body_prefix", "body_entropy",
              "wrapper_endian", "zlib_members", "framed_records"]
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        writer.writerows({field: row[field] for field in fields} for row in rows)

    if args.private_output:
        args.private_output.parent.mkdir(parents=True, exist_ok=True)
        private_fields = ["archive_id", "archive_path", "member_id", "member_path"]
        with args.private_output.open("w", newline="", encoding="utf-8") as handle:
            writer = csv.DictWriter(handle, fieldnames=private_fields)
            writer.writeheader()
            for row in rows:
                writer.writerow({"archive_id": row["archive_id"], "archive_path": row["_archive_path"],
                                 "member_id": row["member_id"], "member_path": row["_member_path"]})

    archive_count = len({row["archive_id"] for row in rows})
    args.summary.write_text(
        "# Archive Survey\n\n"
        f"Found {len(rows)} candidate `.mus` or Finale-recognized extensionless members across {archive_count} "
        "ZIP/StuffIt archives. Members are inventoried by `inventory.py --include-archives` and take part in the "
        "rest of the survey exactly as loose files do, so record and structure findings cover them too. "
        "Archive paths are intentionally omitted; use the local ignored `private/generated/archive_locations.csv` "
        "mapping for archive/member locations. The `unar`/`lsar` 1.10.7 tools can process StuffIt 5 archives, "
        "including resource forks; a complete StuffIt pass may take substantially longer than the ZIP pass.\n\n"
        f"Enigma-banner members: {sum(r['is_enigma_binary'] == 'True' for r in rows)}.\n"
        f"Extensionless candidates: {sum(r['member_name_has_extension'] == 'False' for r in rows)}.\n"
        f"Distinct member contents: {len({r['member_sha256'] for r in rows})}.\n",
        encoding="utf-8",
    )


if __name__ == "__main__":
    main()
