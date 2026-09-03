#!/usr/bin/env python3
"""Inventory legacy Finale files without modifying the source corpus.

A companion is defined by its path: a source ``name.mus`` pairs with
``source-parent/<export-dir-name>/name<export-suffix>`` and with nothing else.
If that file is absent, the source has no companion and is reported ``missing``.

There is deliberately no basename fallback.  A corpus of application installs is
many copies of one product, so the same filename recurs in every version
directory by design; searching for a stem corpus-wide then pairs a Finale 2008
source with a Finale 2011 export and labels the result by how rare the stem is
rather than by how likely the pairing is.  One such run produced 7,266 ambiguous
and 112 "unique" pairings, all of them false.  A guess that is labelled is still
a guess, and a downstream comparison that trusts it reports one version's upgrade
behavior as another's.

The naming convention is not baked in, because it describes one corpus rather
than the format: ``--export-dir-name`` and ``--export-suffix`` are required and
carry no defaults, so a corpus organized differently is inventoried by passing
its own convention.

``--include-archives`` additionally inventories candidate members of ZIP and
StuffIt archives, extracted once into a private cache.  They appear as ordinary
rows distinguished by ``origin``, so every later step in the survey covers them
without knowing that archives exist.  It is opt-in because extraction is by far
the slowest part of a survey.

``--sniff-content`` widens loose-file discovery from the ``.mus`` suffix to
anything whose header is Finale content.  Classic Mac Finale kept the file type
in the resource fork, so its documents commonly have no extension at all and an
extension-only scan silently misses every one of them.  ``--exclude`` drops
subtrees and name patterns that a particular corpus does not want surveyed;
both are corpus conventions, so they are arguments rather than built-in rules.
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
from fnmatch import fnmatch
from pathlib import Path, PurePosixPath

from archive_sources import build_cache, looks_like_mus


# All three banner spellings.  Signature-bearing files carry `Finale(R)` at 0x20;
# the Coda-banner era carries `Finale(TM)` at offset 0 and is the only place its
# version appears; Finale 1.0.0 uses a MacRoman trademark sign (0xAA) with no
# parentheses and follows the version with `ENIGA Structures` (sic) instead of a
# copyright notice.  Matching only `(R)` files them all as "unknown".
BANNER_RE = re.compile(
    rb"Finale(?:\((?:R|TM)\)|\xaa)\s*([^\x00]{1,72}?)(?: Copyright| File Converter| ENIGA Structures)"
)


def norm(value: str) -> str:
    return unicodedata.normalize("NFC", value).casefold()


def make_exclude_filter(patterns: list[str], keep_patterns: list[str] | None = None):
    """Build a predicate over corpus-relative paths from glob patterns.

    A pattern is tested against the path itself *and* against every directory
    above it, so one ``--exclude 'Finale PDK'`` drops that whole subtree instead
    of needing a pattern per file.

    Matching is case-insensitive, and deliberately so.  A corpus that spans
    classic Mac, DOS and Windows filesystems carries the same extension in
    several cases at once — ``.mus``, ``.MUS``, ``.lib``, ``.LIB`` and ``.Lib``
    all occur — so a case-sensitive ``--exclude='*.lib'`` silently excludes part
    of what it names and leaves the rest in the survey.  That failure is invisible
    in the output, which is what makes it worth spending a casefold on.  Suffix
    discovery already casefolds; this keeps the two consistent.
    """
    if not patterns:
        return lambda relative: False

    folded = [pattern.casefold() for pattern in patterns]
    kept = [pattern.casefold() for pattern in (keep_patterns or [])]

    def matches(relative: str, against: list[str]) -> bool:
        path = PurePosixPath(relative.casefold())
        candidates = [str(path)] + [str(parent) for parent in path.parents if str(parent) != "."]
        return any(fnmatch(candidate, pattern) for candidate in candidates for pattern in against)

    def excluded(relative: str) -> bool:
        # A keep pattern overrides an exclusion, because the two are not always
        # separable by directory. Finale's stock font-default documents sit inside the
        # library tree that otherwise holds no documents at all, so excluding the tree
        # by path would silently drop the specimens that tree was kept for.
        if kept and matches(relative, kept):
            return False
        return matches(relative, folded)

    return excluded


def discover_sources(root: Path, excluded, sniff_content: bool) -> list[tuple[Path, str]]:
    """Find loose specimens, reporting how each one was recognized.

    ``suffix`` means the name ended in ``.mus``.  ``sniffed`` means the name said
    nothing and the header did — the only way to see classic Mac documents,
    whose type lived in the resource fork rather than the extension.
    """
    found: list[tuple[Path, str]] = []
    for path in root.rglob("*"):
        name = path.name
        # AppleDouble sidecars hold the resource fork of the file beside them,
        # never a document of their own, and .DS_Store is Finder state.
        if name.startswith("._") or name == ".DS_Store":
            continue
        if not path.is_file():
            continue
        relative = path.relative_to(root).as_posix()
        if excluded(relative):
            continue
        if path.suffix.casefold() == ".mus":
            found.append((path, "suffix"))
            continue
        if not sniff_content:
            continue
        try:
            with path.open("rb") as handle:
                header = handle.read(256)
        except OSError:
            continue
        if looks_like_mus(header):
            found.append((path, "sniffed"))
    return sorted(found, key=lambda item: norm(str(item[0].relative_to(root))))


def corpus_fingerprint(loose_rows: list[dict]) -> str:
    """Name the loose source population by its contents, never by its paths."""
    digests = sorted(str(row["source_sha256"]) for row in loose_rows)
    combined = hashlib.sha256("\n".join(digests).encode("utf-8")).hexdigest()
    return f"corpus-{combined[:16]}"


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


def find_export(
    source: Path,
    export_dir_name: str,
    export_suffix: str,
) -> tuple[Path | None, str]:
    """Pair a source with its companion by path, or report that it has none."""
    expected = source.parent / export_dir_name / f"{source.stem}{export_suffix}"
    if expected.is_file():
        return expected, "adjacent-exact"
    return None, "missing"


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
        "--sniff-content",
        action="store_true",
        help="Also inventory loose files whose header is Finale content but whose name is not *.mus",
    )
    parser.add_argument(
        "--exclude",
        action="append",
        default=[],
        metavar="GLOB",
        help="Skip paths matching this glob, or anything beneath a directory matching it. "
             "Applied to corpus-relative loose paths and archive member paths; repeatable. "
             "E.g. --exclude='*/Libraries*'",
    )
    parser.add_argument(
        "--keep",
        action="append",
        default=[],
        metavar="GLOB",
        help="Re-admit paths matching this glob even when --exclude would drop them; "
             "repeatable. E.g. --exclude='*/Libraries*' --keep='*Font Default*'",
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

    excluded = make_exclude_filter(args.exclude, args.keep)
    sources = discover_sources(root, excluded, args.sniff_content)
    exports = sorted(
        (path for path in root.rglob(f"*{export_suffix}")
         if not excluded(path.relative_to(root).as_posix())),
        key=lambda path: norm(str(path.relative_to(root))),
    )
    # A specimen is anything to inventory: a loose file, or a cached archive
    # member.  Both are real files by this point, so the loop below does not
    # care which is which except when pairing exports.
    specimens: list[dict[str, object]] = [
        {"origin": "filesystem", "discovery": discovery, "path": source,
         "relative": str(source.relative_to(root)),
         "archive_id": "", "archive_path": "", "member_path": ""}
        for source, discovery in sources
    ]

    archive_stats: dict[str, int] = {}
    if args.include_archives:
        cache_dir = (args.archive_cache or (output_dir / "archive_cache")).resolve()
        members, archive_stats = build_cache(
            root, cache_dir, refresh=args.refresh_archive_cache, excluded=excluded
        )
        print(f"archives: {archive_stats['archives']} scanned, {archive_stats['extracted']} extracted, "
              f"{archive_stats['reused']} reused from cache, {archive_stats['unreadable']} unreadable")
        for member in sorted(members, key=lambda m: (norm(m["archive_filename"]), norm(m["member_path"]))):
            if excluded(member["member_path"]):
                continue
            path = cache_dir / f"{member['member_id']}.mus"
            if not path.is_file():
                continue
            specimens.append({
                "origin": "archive",
                "discovery": "archive-member",
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
        # Archive members live in a cache rather than beside a companion
        # directory, so the path convention cannot apply to them at all.
        if specimen["origin"] == "filesystem":
            export, match_kind = find_export(source, export_dir_name, export_suffix)
        else:
            export, match_kind = None, "not-applicable"
        stat = source.stat()
        row: dict[str, object] = {
            "index": index,
            "origin": specimen["origin"],
            "discovery": specimen["discovery"],
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
    discoveries = Counter(str(row["discovery"]) for row in rows)
    loose = [row for row in rows if row["origin"] == "filesystem"]
    members = [row for row in rows if row["origin"] == "archive"]
    summary = {
        "corpus_root": str(root),
        "export_dir_name": export_dir_name,
        "export_suffix": export_suffix,
        # Recorded because both change what "the corpus" means: a survey run
        # with exclusions covers less than the tree it names, and one run
        # without sniffing misses every extensionless document in it. Numbers
        # from two runs are only comparable when these agree.
        "sniff_content": bool(args.sniff_content),
        "exclude": list(args.exclude),
        "keep": list(args.keep),
        # source_* count loose files only, so these stay comparable with surveys
        # taken before archives could be included.
        "source_count": len(sources),
        # A content-derived name for the loose source population, in the same shape as the
        # mus-/arc- tokens: sorted so the filesystem walk order cannot change it, duplicates
        # kept so that adding any file moves it, and archive members excluded so that a survey
        # taken with and without archives still fingerprints the same corpus.
        "corpus_fingerprint": corpus_fingerprint(loose),
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
        "discovery_counts": dict(sorted(discoveries.items())),
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
        f"- Legacy loose sources: **{len(sources)}** "
        f"({discoveries.get('suffix', 0)} by `.mus` suffix, {discoveries.get('sniffed', 0)} by content)",
        f"- `{export_suffix}` files present: **{len(exports)}**",
        f"- Legacy sources matched to an export: **{summary['matched_source_count']}**",
        "",
        "The saving product is read from the binary header. Timestamps and path names are supporting provenance only. "
        "SHA-256 values cover the complete data fork. Full machine-readable paths and header bytes are in "
        "this directory's `corpus_inventory.csv` (local-only).",
        "",]
    if args.exclude:
        markdown.extend([
            "Excluded from the scan, so this describes less than the tree it names: "
            + ", ".join(f"`{pattern}`" for pattern in args.exclude),
            "",
        ])
    markdown.extend([
        "## Saving-product distribution",
        "",
        "| Header product | Files |",
        "|---|---:|",
    ])
    markdown.extend(f"| {key} | {value} |" for key, value in sorted(products.items()))
    markdown.extend([
        "",
        "## Files examined",
        "",
        "| # | Source (relative to corpus root) | Bytes | SHA-256 | Header product | Found by | Export | Match |",
        "|---:|---|---:|---|---|---|---|---|",
    ])
    for row in rows:
        source_rel = str(row["source_relative"]).replace("|", "\\|")
        export_rel = str(row["export_relative"]).replace("|", "\\|") or "—"
        markdown.append(
            f"| {row['index']} | `{source_rel}` | {row['source_size']} | `{row['source_sha256']}` | "
            f"{row['saving_product']} | {row['discovery']} | `{export_rel}` | {row['export_match']} |"
        )
    # Written inside the output directory rather than beside it, so two surveys
    # rebuilt into their own private directories cannot overwrite each other's
    # copy.  This is the path-bearing inventory; the publishable one comes from
    # render_corpus_inventory.py.
    (output_dir / "CORPUS_INVENTORY.md").write_text("\n".join(markdown) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
