#!/usr/bin/env python3
"""Inventory legacy Finale files without modifying the source corpus.

Matching assumption (reported, not hidden): a source ``name.mus`` normally maps
to ``source-parent/<export-dir-name>/name<export-suffix>``.  A normalized,
case-insensitive basename search is used only when that exact convention does
not match.

The naming convention is not baked in, because it describes one corpus rather
than the format: ``--export-dir-name`` and ``--export-suffix`` are required and
carry no defaults, so a corpus organized differently is inventoried by passing
its own convention.

``--include-archives`` additionally inventories candidate members of ZIP and
StuffIt archives, extracted once into a private cache.  They appear as ordinary
rows distinguished by ``origin``, so every later step in the survey covers them
without knowing that archives exist.  It is opt-in because extraction is by far
the slowest part of a survey.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
import unicodedata
from collections import Counter
from datetime import datetime
from pathlib import Path

from archive_sources import build_cache


# Both banner spellings. Signature-bearing files carry `Finale(R)` at 0x20; the
# Coda-banner era carries `Finale(TM)` at offset 0 and is the only place its
# version appears, so matching only `(R)` files them all as "unknown".
BANNER_RE = re.compile(rb"Finale\((?:R|TM)\)\s*([^\x00]{1,72}?)(?: Copyright| File Converter)")


def norm(value: str) -> str:
    return unicodedata.normalize("NFC", value).casefold()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def parse_banner(header: bytes) -> tuple[str, str]:
    match = BANNER_RE.search(header[:0x80])
    if match is None:
        # Keep reporting the conventional banner slot so an unrecognized file
        # still shows whatever text sits there.
        return header[0x20:0x60].split(b"\x00", 1)[0].decode("mac_roman", "replace"), "unknown"
    # Read the banner from wherever it was found rather than a fixed offset,
    # since the two eras place it differently.
    raw = header[match.start() : match.start() + 0x40].split(b"\x00", 1)[0].decode("mac_roman", "replace")
    return raw, match.group(1).decode("ascii", "replace")


def export_source_dir(candidate: Path, export_dir_depth: int) -> Path:
    """Directory a candidate export would sit beside, undoing the export dir."""
    directory = candidate.parent
    for _ in range(export_dir_depth):
        directory = directory.parent
    return directory


def find_export(
    source: Path,
    exports_by_stem: dict[str, list[Path]],
    export_dir_name: str,
    export_suffix: str,
) -> tuple[Path | None, str]:
    expected = source.parent / export_dir_name / f"{source.stem}{export_suffix}"
    if expected.is_file():
        return expected, "adjacent-exact"
    candidates = exports_by_stem.get(norm(source.stem), [])
    if not candidates:
        return None, "missing"
    # Prefer the candidate with the longest common parent prefix.  Ambiguous
    # fallback matches remain visibly labelled and are not treated as exact.
    source_parts = source.parent.parts
    export_dir_depth = len(Path(export_dir_name).parts)
    ranked: list[tuple[int, str, Path]] = []
    for candidate in candidates:
        common = 0
        for left, right in zip(source_parts, export_source_dir(candidate, export_dir_depth).parts):
            if norm(left) != norm(right):
                break
            common += 1
        ranked.append((common, str(candidate), candidate))
    ranked.sort(key=lambda item: (-item[0], item[1]))
    best = ranked[0]
    ties = [item for item in ranked if item[0] == best[0]]
    return best[2], "fallback-unique" if len(ties) == 1 else "fallback-ambiguous"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path, help="Corpus root")
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument(
        "--export-dir-name",
        required=True,
        help="Directory beside each source holding its Finale 27 exports, e.g. '-exports'",
    )
    parser.add_argument(
        "--export-suffix",
        required=True,
        help="Suffix identifying an export, e.g. '.fin27.musx'",
    )
    parser.add_argument(
        "--include-archives",
        action="store_true",
        help="Also inventory candidate members of ZIP/StuffIt archives (slow on first run)",
    )
    parser.add_argument(
        "--archive-cache",
        type=Path,
        help="Where extracted members are cached; defaults to <output-dir>/archive_cache",
    )
    parser.add_argument(
        "--refresh-archive-cache",
        action="store_true",
        help="Re-extract every archive instead of reusing the cache",
    )
    args = parser.parse_args()
    export_dir_name = args.export_dir_name
    export_suffix = args.export_suffix
    root = args.root.resolve()
    output_dir = args.output_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)

    sources = sorted(
        (path for path in root.rglob("*") if path.is_file() and path.suffix.casefold() == ".mus"),
        key=lambda path: norm(str(path.relative_to(root))),
    )
    exports = sorted(root.rglob(f"*{export_suffix}"), key=lambda path: norm(str(path.relative_to(root))))
    exports_by_stem: dict[str, list[Path]] = {}
    for export in exports:
        stem = export.name[: -len(export_suffix)]
        exports_by_stem.setdefault(norm(stem), []).append(export)

    # A specimen is anything to inventory: a loose file, or a cached archive
    # member.  Both are real files by this point, so the loop below does not
    # care which is which except when pairing exports.
    specimens: list[dict[str, object]] = [
        {"origin": "filesystem", "path": source, "relative": str(source.relative_to(root)),
         "archive_id": "", "archive_path": "", "member_path": ""}
        for source in sources
    ]

    archive_stats: dict[str, int] = {}
    if args.include_archives:
        cache_dir = (args.archive_cache or (output_dir / "archive_cache")).resolve()
        members, archive_stats = build_cache(root, cache_dir, refresh=args.refresh_archive_cache)
        print(f"archives: {archive_stats['archives']} scanned, {archive_stats['extracted']} extracted, "
              f"{archive_stats['reused']} reused from cache, {archive_stats['unreadable']} unreadable")
        for member in sorted(members, key=lambda m: (norm(m["archive_filename"]), norm(m["member_path"]))):
            path = cache_dir / f"{member['member_id']}.mus"
            if not path.is_file():
                continue
            specimens.append({
                "origin": "archive",
                "path": path,
                # "!" separates container from member. Path().name still yields
                # the member's basename, which is what gets published.
                "relative": f"{member['archive_filename']}!{member['member_path']}",
                "archive_id": member["archive_id"],
                "archive_path": member["archive_path"],
                "member_path": member["member_path"],
            })

    rows: list[dict[str, object]] = []
    export_hash_cache: dict[Path, str] = {}
    for index, specimen in enumerate(specimens, 1):
        source = specimen["path"]
        with source.open("rb") as handle:
            header = handle.read(0x200)
        banner, product = parse_banner(header)
        # Archive members have no export counterpart, and the fallback matcher
        # ranks by path prefix, so letting it run would invent a pairing from a
        # coincidental basename match against some unrelated loose export.
        if specimen["origin"] == "filesystem":
            export, match_kind = find_export(source, exports_by_stem, export_dir_name, export_suffix)
        else:
            export, match_kind = None, "not-applicable"
        stat = source.stat()
        row: dict[str, object] = {
            "index": index,
            "origin": specimen["origin"],
            "source_relative": specimen["relative"],
            "source_path": str(source),
            "archive_id": specimen["archive_id"],
            "archive_path": specimen["archive_path"],
            "member_path": specimen["member_path"],
            "source_size": stat.st_size,
            "source_sha256": sha256(source),
            # A cached member's mtime is its extraction time, which says nothing
            # about the document, so it is left empty rather than made up.
            "source_mtime": (datetime.fromtimestamp(stat.st_mtime).astimezone().isoformat()
                             if specimen["origin"] == "filesystem" else ""),
            "signature_ok": header.startswith(b"ENIGMA BINARY FILE\x00"),
            "banner": banner,
            "saving_product": product,
            "header_0x60_0xb0": header[0x60:0xB0].hex(),
            "export_match": match_kind,
            "export_relative": str(export.relative_to(root)) if export else "",
            "export_path": str(export) if export else "",
            "export_size": export.stat().st_size if export else "",
            "export_sha256": "",
        }
        if export:
            if export not in export_hash_cache:
                export_hash_cache[export] = sha256(export)
            row["export_sha256"] = export_hash_cache[export]
        rows.append(row)

    fieldnames = list(rows[0]) if rows else []
    with (output_dir / "corpus_inventory.csv").open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)

    products = Counter(str(row["saving_product"]) for row in rows)
    matches = Counter(str(row["export_match"]) for row in rows)
    origins = Counter(str(row["origin"]) for row in rows)
    loose = [row for row in rows if row["origin"] == "filesystem"]
    members = [row for row in rows if row["origin"] == "archive"]
    summary = {
        "corpus_root": str(root),
        "export_dir_name": export_dir_name,
        "export_suffix": export_suffix,
        # source_* count loose files only, so these stay comparable with surveys
        # taken before archives could be included.
        "source_count": len(sources),
        "source_bytes": sum(int(row["source_size"]) for row in loose),
        "export_count": len(exports),
        "matched_source_count": sum(bool(row["export_path"]) for row in rows),
        "unique_matched_export_count": len(export_hash_cache),
        "matched_export_bytes": sum(path.stat().st_size for path in export_hash_cache),
        "archives_included": bool(args.include_archives),
        "archive_member_count": len(members),
        "archive_member_bytes": sum(int(row["source_size"]) for row in members),
        "distinct_archive_member_count": len({str(row["source_sha256"]) for row in members}),
        "archive_scan": archive_stats,
        "specimen_count": len(rows),
        "origin_counts": dict(sorted(origins.items())),
        "saving_product_counts": dict(sorted(products.items())),
        "match_counts": dict(sorted(matches.items())),
    }
    (output_dir / "inventory_summary.json").write_text(
        json.dumps(summary, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )

    markdown = [
        "# Corpus Inventory",
        "",
        f"Generated from `{root}` by `scripts/inventory.py`.",
        "",
        f"- Legacy `.mus` files: **{len(sources)}**",
        f"- `{export_suffix}` files present: **{len(exports)}**",
        f"- Legacy sources matched to an export: **{summary['matched_source_count']}**",
        "",
        "The saving product is read from the binary header. Timestamps and path names are supporting provenance only. "
        "SHA-256 values cover the complete data fork. Full machine-readable paths and header bytes are in "
        "`private/generated/corpus_inventory.csv` (local-only).",
        "",
        "## Saving-product distribution",
        "",
        "| Header product | Files |",
        "|---|---:|",
    ]
    markdown.extend(f"| {key} | {value} |" for key, value in sorted(products.items()))
    markdown.extend([
        "",
        "## Files examined",
        "",
        "| # | Source (relative to corpus root) | Bytes | SHA-256 | Header product | Export | Match |",
        "|---:|---|---:|---|---|---|---|",
    ])
    for row in rows:
        source_rel = str(row["source_relative"]).replace("|", "\\|")
        export_rel = str(row["export_relative"]).replace("|", "\\|") or "—"
        markdown.append(
            f"| {row['index']} | `{source_rel}` | {row['source_size']} | `{row['source_sha256']}` | "
            f"{row['saving_product']} | `{export_rel}` | {row['export_match']} |"
        )
    (output_dir.parent / "CORPUS_INVENTORY.md").write_text("\n".join(markdown) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
