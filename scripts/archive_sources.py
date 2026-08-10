#!/usr/bin/env python3
"""Discover Finale-like members inside ZIP and StuffIt archives.

Archive members are evidence like any other file, but they are not files on
disk, so nothing downstream of the inventory could read them.  This extracts
each distinct member once into a private cache keyed by its content hash, which
lets every later step treat an archive member exactly as it treats a loose file.

Extraction is the slowest part of a survey and the cache makes it a one-time
cost: a rerun re-reads the cache instead of unpacking StuffIt again.  Members are
deduplicated by content, so a file present in several archives is stored once.

The corpus is never written to.  StuffIt extraction goes to a temporary
directory and the cache lives under private/, which git ignores.
"""

from __future__ import annotations

import csv
import hashlib
import os
import re
import subprocess
import tempfile
import zipfile
from pathlib import Path

INDEX_NAME = "cache_index.csv"
INDEX_FIELDS = ["member_id", "archive_id", "archive_filename", "archive_path", "member_path", "member_size",
                "archive_unrecognized"]

ARCHIVE_SUFFIXES = {".zip", ".sit"}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def member_id(data: bytes) -> str:
    """Content-derived id, the same convention loose sources use.

    A member and a loose file with identical bytes therefore share an id, which
    is what lets one corpus corroborate another.
    """
    return "mus-" + digest(data)[:16]


def archive_id(data: bytes) -> str:
    return "arc-" + digest(data)[:16]


SIGNATURE = b"ENIGMA BINARY FILE"
ETF_SIGNATURE = b"ENIGMA TRANSPORTABLE FILE"
# All three banner spellings: `Finale(R)`, the Coda-era `Finale(TM)`, and Finale
# 1.0.0's MacRoman trademark sign (0xAA) with no parentheses.  Keep this in step
# with BANNER_RE in inventory.py; a spelling missing here means those files are
# never cached, so the parser there never gets the chance to read them.
BANNER_RE = re.compile(rb"Finale(?:\((?:R|TM)\)|\xaa)")


def likely_member(name: str) -> bool:
    """Cheap name filter: a .mus suffix, or no extension at all.

    Classic Mac files often carry no extension because the type lived in the
    resource fork, so extensionless members cannot be skipped on name alone.
    They still have to pass ``looks_like_mus`` before anything is cached.
    """
    base = name.rsplit("/", 1)[-1]
    if not base or base.startswith("._") or "/__MACOSX/" in f"/{name}":
        return False
    suffix = Path(base).suffix.lower()
    return suffix == ".mus" or not suffix


def looks_like_mus(data: bytes) -> bool:
    """Recognize legacy Finale content by its banner.

    Banner-era files open with the ENIGMA signature; the older Coda-banner files
    carry a ``Finale(R)``/``Finale(TM)`` string near the start instead.  Checking
    content is what keeps READMEs and other extensionless members out of the
    cache.

    ETF is excluded explicitly.  An ETF is Finale's *text* interchange format and
    it quotes the same product banner on its fourth line, so a banner search
    alone accepts it — including the extensionless ETFs that classic Mac Finale
    wrote, which no extension rule would catch.  It is a real Finale file and
    valuable evidence, but it is not a MUS binary, and admitting it to a MUS
    inventory only produces rows that every reader must reject.
    """
    if data.startswith(ETF_SIGNATURE):
        return False
    return data.startswith(SIGNATURE) or BANNER_RE.search(data[:256]) is not None


def is_cacheable(name: str, data: bytes) -> bool:
    """A .mus member is taken on its name; anything else must look like one."""
    if Path(name.rsplit("/", 1)[-1]).suffix.lower() == ".mus":
        return True
    return looks_like_mus(data)


def specimen_filename(row: dict) -> str:
    """Publishable basename for an inventory row, loose or archived.

    ``source_relative`` for a member is ``<archive>!<member path>``, and a member
    at an archive's root has no slash to split on, so the container name would
    leak into the published filename.  Use the member path when there is one.
    """
    if row.get("origin") == "archive" and row.get("member_path"):
        return row["member_path"].rsplit("/", 1)[-1]
    return row["source_relative"].rsplit("/", 1)[-1]


def find_archives(root: Path, excluded=None) -> list[Path]:
    """Locate archives under root, honouring the caller's exclusions.

    ``excluded`` takes a POSIX path relative to root.  The caller owns the
    pattern semantics; this only has to apply the same decision to archives that
    it applies to loose files, so an excluded subtree stays excluded whether or
    not its contents happen to be zipped.
    """
    found: list[Path] = []
    for directory, _, names in os.walk(root):
        for name in names:
            path = Path(directory) / name
            if path.suffix.lower() not in ARCHIVE_SUFFIXES:
                continue
            if excluded is not None and excluded(path.relative_to(root).as_posix()):
                continue
            found.append(path)
    return sorted(found)


def read_members(archive: Path) -> list[tuple[str, bytes]]:
    """Return (member_path, data) for candidate members, without touching the corpus."""
    if archive.suffix.lower() == ".zip":
        members: list[tuple[str, bytes]] = []
        with zipfile.ZipFile(archive) as handle:
            for info in handle.infolist():
                if info.is_dir() or not likely_member(info.filename):
                    continue
                members.append((info.filename, handle.read(info)))
        return members

    members = []
    with tempfile.TemporaryDirectory(prefix="mus_sit_") as temp:
        result = subprocess.run(
            ["unar", "-quiet", "-p", "", "-o", temp, str(archive)],
            capture_output=True, text=True, check=False,
        )
        if result.returncode != 0:
            raise RuntimeError(result.stderr.strip() or "unar failed")
        extracted_root = Path(temp) / archive.stem
        if not extracted_root.exists():
            extracted_root = Path(temp)
        for extracted in sorted(extracted_root.rglob("*")):
            if not extracted.is_file() or not likely_member(extracted.name):
                continue
            members.append((str(extracted.relative_to(extracted_root)), extracted.read_bytes()))
    return members


def load_index(cache_dir: Path) -> list[dict[str, str]]:
    index = cache_dir / INDEX_NAME
    if not index.is_file():
        return []
    with index.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def cached_path(cache_dir: Path, mid: str) -> Path:
    return cache_dir / f"{mid}.mus"


def build_cache(root: Path, cache_dir: Path, refresh: bool = False,
                excluded=None) -> tuple[list[dict[str, str]], dict[str, int]]:
    """Extract candidate members into cache_dir, reusing what is already there.

    Returns one row per (archive, member) occurrence and a stats mapping.  Rows
    are occurrences rather than distinct members: the same file appearing in
    several archives is one cached blob and several rows, which is what records
    where a member was found.

    The on-disk index additionally holds a sentinel row for any archive that
    yielded nothing cacheable, and remembers per archive how many members were
    rejected.  Without the sentinel such an archive looks unseen and is
    unpacked again every run; without the remembered count the rejection total
    would describe this run's extraction work rather than the corpus.
    """
    cache_dir.mkdir(parents=True, exist_ok=True)
    previous = [] if refresh else load_index(cache_dir)
    by_archive: dict[str, list[dict[str, str]]] = {}
    for row in previous:
        by_archive.setdefault(row["archive_id"], []).append(row)

    index_rows: list[dict[str, str]] = []
    rows: list[dict[str, str]] = []
    stats = {"archives": 0, "reused": 0, "extracted": 0, "unreadable": 0, "unrecognized": 0, "members": 0}
    for archive in find_archives(root, excluded):
        try:
            aid = archive_id(archive.read_bytes())
        except OSError:
            stats["unreadable"] += 1
            continue
        stats["archives"] += 1

        cached = by_archive.get(aid)
        if cached is not None and all(
            cached_path(cache_dir, row["member_id"]).is_file() for row in cached if row["member_id"]
        ):
            # An archive whose members were all rejected still gets an index
            # entry, so it is recognized as seen instead of being unpacked
            # again on every run.
            kept = [row for row in cached if row["member_id"]]
            index_rows.extend(cached)
            rows.extend(kept)
            stats["reused"] += 1
            stats["members"] += len(kept)
            # Carry the remembered count so the total describes the corpus
            # rather than however much extraction this particular run did.
            stats["unrecognized"] += int(cached[0].get("archive_unrecognized") or 0)
            continue

        try:
            members = read_members(archive)
        except (zipfile.BadZipFile, OSError, RuntimeError):
            stats["unreadable"] += 1
            continue
        stats["extracted"] += 1

        kept_rows: list[dict[str, str]] = []
        rejected = 0
        for member_path, data in members:
            if not is_cacheable(member_path, data):
                rejected += 1
                continue
            mid = member_id(data)
            target = cached_path(cache_dir, mid)
            if not target.is_file():
                target.write_bytes(data)
            kept_rows.append({
                "member_id": mid,
                "archive_id": aid,
                "archive_filename": archive.name,
                "archive_path": str(archive),
                "member_path": member_path,
                "member_size": str(len(data)),
                "archive_unrecognized": str(rejected),
            })
        stats["unrecognized"] += rejected
        stats["members"] += len(kept_rows)
        for row in kept_rows:
            row["archive_unrecognized"] = str(rejected)
        if kept_rows:
            index_rows.extend(kept_rows)
            rows.extend(kept_rows)
        else:
            # Sentinel: nothing worth keeping, but remember having looked.
            index_rows.append({
                "member_id": "",
                "archive_id": aid,
                "archive_filename": archive.name,
                "archive_path": str(archive),
                "member_path": "",
                "member_size": "0",
                "archive_unrecognized": str(rejected),
            })

    sort_key = lambda row: (row["archive_id"], row["member_path"], row["member_id"])
    rows.sort(key=sort_key)
    index_rows.sort(key=sort_key)
    with (cache_dir / INDEX_NAME).open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=INDEX_FIELDS)
        writer.writeheader()
        writer.writerows(index_rows)
    return rows, stats
