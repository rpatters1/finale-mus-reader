#!/usr/bin/env python3
"""Summarize recovery_coverage_probe's JSON Lines output as plain text tables.

No LLM in the loop: every table here is a deterministic reduction over the probe's own
output, meant to replace asking an assistant to eyeball a jq query and describe what it
found. Two things are summarized:

  - what the reader recovered: status/epoch counts, diagnostic message frequency, and
    failure reasons, all counted across every row in the file.
  - how it compares to Finale's own companion, for rows the probe ran with a
    `#companion:`-declared corpus (see recovery_coverage_probe.cpp): same /
    expected-difference / unexpected-difference / reader-only / companion-only counts per
    surveyed class.

The companion comparison walks every leaf under each surveyor's output (skipping
`origin_*` annotations, which exist only on the source side and would differ on every row
by construction) and classifies each non-matching leaf through EXPECTED_DIFFERENCES below.
That table starts small and is meant to grow: an "unexpected" count is not necessarily a
regression, it is the next entry to characterize and, if it is a real known difference, add
a rule for. This script is the maintained home for those exceptions.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from collections import Counter
from pathlib import Path
from typing import Any, Iterable, Iterator

# Row keys that describe the row/import itself rather than surveyed content. Excluded from
# both the per-class breakdown and the recursive companion diff.
METADATA_KEYS = {
    "corpus_id", "status", "epoch", "saving_product", "source_version",
    "warning_count", "diagnostics", "duration_ms", "companion", "finder_type", "error",
}


def is_source_only_key(key: str) -> bool:
    """True for a key that describes the source side's own recovery process rather than a
    value Finale itself stores -- field-origin annotations (both naming conventions actually
    in use: the common `origin_fieldName` prefix, and font_options.cpp's `fieldName_origin`
    suffix) and font_options.cpp's raw legacy-byte decode positions (`*_block_offset`,
    `*_decoded_field_offset`). The companion's ImportReport is always empty and it was never
    decoded from legacy bytes at all, so every one of these would differ on every row by
    construction -- not a value to compare, so excluded outright rather than classified.
    """
    return (key.startswith("origin_") or key.endswith("_origin")
        or key.endswith("_block_offset") or key.endswith("_decoded_field_offset"))


Category = str  # "differs" | "reader_only" | "companion_only" ("same" is never classified)

# (path_regex, categories, origins, label): a leaf whose dotted path matches, whose
# category is in `categories` (None means any), and whose source-side origin annotation is
# in `origins` (None means don't care) is an intended difference or a structurally
# one-sided value, not a regression candidate. Seeded from real unexpected-difference runs
# against tracked-evidence and exceptions carried forward from the retired options comparator.
# Scoping by category and origin
# matters: a leaf that is reader_only or companion_only because one side's collection is
# longer than the other is a different claim from the same path *differing* where both
# sides have a value, and a rule should not paper over the second just because the first is
# expected; likewise a value this reader actually recovered from the source disagreeing
# with the companion is a different claim from a synthesized reference default disagreeing
# with it, even at the same path.
EXPECTED_DIFFERENCES: list[tuple[str, set[Category] | None, set[str | None] | None, str]] = [
    (r"^font_options\.tuples\b", None, None,
        "font_options.tuples is built from this reader's own field-provenance report "
        "(ImportReport.fields), which a companion -- never imported by this reader -- has "
        "none of; not a value comparison at all, so every leaf under it reads reader_only"),
    (r"^stem_options\.stem_connections\[", {"companion_only"}, None,
        "past-terminator: musxdom completes the stem-connection table past the first "
        "symbol-less entry, where this reader stops (see stem_options.cpp)"),
    (r"\bshape_id$", {"differs"}, {"finale27-default"},
        "a shape_id the source side never recovered -- it is the pinned Finale 27 "
        "baseline's own default reference -- so it names a shape in that baseline's own "
        "numbering, not this specific companion's; shape ids are reassigned per save (see "
        "the cmper instability note on LIST_MATCH_KEY_OVERRIDES), so the two numbers "
        "agreeing was never the right test. What should agree, and isn't checked here yet, "
        "is whether the two referenced shapes are the same shape."),
]

# ShapeDefInstructionType::SetFont's ordinal (see musxdom's ShapeDesigner.h); its first of
# three data items is a font id (font id, size, efx). No other shape instruction references
# a font.
_SET_FONT_INSTRUCTION_TYPE = 20

# ShapeDefInstructionType::StartObject's ordinal -- the instruction realign_shape_cmpers()
# tolerates as an unmatched leading addition on a pre-Finale-2000 shape (see its own note).
_START_OBJECT_INSTRUCTION_TYPE = 25


def shape_set_font_paths(row: dict) -> dict[str, int]:
    """Maps the full leaf path of every SetFont font-id data item, under this row's own
    shape_data, to the font id stored there.

    ShapeDef, ShapeInstructionList, and ShapeData are three independent, cmper-keyed pools
    -- tools/coverage/surveyors/others/shape_definitions.cpp's own header comment says
    joining them (resolving a shape's instructionList/dataList to find which data item is
    which) is a question for "an aggregator," deliberately left undone by the surveyor
    itself. This is that join, for exactly the one purpose that needs it here: past that
    join, a shape's font-id data items are otherwise indistinguishable from any other
    integer in shape_data's flat values array. It walks each instruction's own `num_data`
    to find its data items' offsets, the same bookkeeping
    src/import/others/shape_definitions.cpp's import-time earlyLineWidths pass already does
    for a different instruction (LineWidth) and a different purpose (unit conversion) --
    small enough, and specific enough to this one instruction type, that duplicating the
    walk here beats teaching the surveyor to cross its own pool boundaries.

    Works identically on a source row or a companion object, since both carry the same
    shape_defs/shape_instruction_lists/shape_data shape. Only the source side's positions
    are used to decide *whether* a differing value is excused (see compare_row()); the
    companion's value at that same position is trusted to mean the same thing without
    independently re-deriving it from the companion's own instruction stream. That holds
    whenever the two streams agree instruction-for-instruction, which is the common case,
    but not always -- when they diverge, the companion's raw value at that position is
    whatever its own, differently-shaped stream put there, not a font id, and
    resolve_font_name() correctly fails to find it rather than reporting a wrong name.
    """
    instructions_by_cmper = {lst["cmper"]: lst["instructions"]
        for lst in (row.get("shape_instruction_lists") or {}).get("lists") or []}
    values_by_cmper = {buf["cmper"]: buf["values"]
        for buf in (row.get("shape_data") or {}).get("buffers") or []}
    paths: dict[str, int] = {}
    for shape in row.get("shape_defs") or []:
        instructions = instructions_by_cmper.get(shape.get("instruction_list"))
        values = values_by_cmper.get(shape.get("data_list"))
        if not instructions or not values:
            continue
        offset = 0
        for instruction in instructions:
            if instruction.get("type") == _SET_FONT_INSTRUCTION_TYPE and offset < len(values):
                path = (f"shape_data.buffers[cmper={shape['data_list']}]"
                    f".values[{offset}].value")
                paths[path] = values[offset]["value"]
            offset += instruction.get("num_data", 0)
    return paths


def resolve_font_name(row: dict, font_id: Any) -> str | None:
    """The normalized_name of the font_definitions entry `font_id` names in `row` (source
    row or companion object), or None if `row` has no such entry -- a document only ever
    resolves a font id against its own font table, never another document's."""
    definitions = (row.get("font_definitions") or {}).get("definitions") or []
    match = next((d for d in definitions if d.get("cmper") == font_id), None)
    return match.get("normalized_name") if match else None


def _instruction_signature(instruction_list: dict | None) -> tuple:
    """An instruction list's own (type, num_data) sequence -- its content, independent of
    which cmper names it or which shape (if any) references it."""
    if not instruction_list:
        return ()
    return tuple((i.get("type"), i.get("num_data"))
        for i in instruction_list.get("instructions") or [])


def _shape_font_id_positions(row: dict) -> dict[int, set[int]]:
    """{data_list cmper: {value indices that are a SetFont font id}}, from
    shape_set_font_paths()'s flat path strings -- grouped by buffer here because
    _data_signature() below needs "which indices to mask" per buffer, not per leaf."""
    positions: dict[int, set[int]] = {}
    for path in shape_set_font_paths(row):
        cmper_part, index_part = path.split("].values[")
        cmper = int(cmper_part.split("cmper=")[1])
        index = int(index_part.split("]")[0])
        positions.setdefault(cmper, set()).add(index)
    return positions


def _data_signature(buffer: dict, masked_indices: set[int], consumed: int | None) -> tuple:
    """A data buffer's own values, with its SetFont font-id slots (if any) replaced by a
    placeholder so two buffers that are really the same shape's data still match even
    though a font id is expected to differ (see shape_set_font_paths()), and truncated to
    its first `consumed` values when known (see _consumed_lengths()) -- an instruction
    stream's own num_data never has to add up to exactly how many values its data list
    holds, and a value past what any instruction reads is unused padding, not content, and
    not guaranteed to be the same length on both sides (confirmed directly:
    MultimeasureRestOptions.shape_def carried one such trailing value on the source side
    and two on the companion's)."""
    values = buffer.get("values") or []
    if consumed is not None:
        values = values[:consumed]
    return tuple(None if v.get("index") in masked_indices else v.get("value") for v in values)


def _consumed_lengths(shapes: list[dict], lists_by_cmper: dict[int, dict]) -> dict[int, int]:
    """{data_list cmper: total num_data its own shape's instructions actually consume}.
    setdefault() rather than overwrite: if more than one shape ever names the same data
    list (not expected, but not forbidden either), the first one found decides -- this
    only needs one answer, not a definitive one, since consumed-vs-padding is unaffected
    by which shape asked as long as they agree, and if they do not, some length has to
    win rather than none.
    """
    lengths: dict[int, int] = {}
    for shape in shapes:
        instruction_list = lists_by_cmper.get(shape.get("instruction_list"))
        if not instruction_list:
            continue
        consumed = sum(i.get("num_data", 0) for i in instruction_list.get("instructions") or [])
        lengths.setdefault(shape.get("data_list"), consumed)
    return lengths


def realign_shape_cmpers(source: dict, companion: dict) -> None:
    """Mutates `companion` in place so its shape_defs/shape_instruction_lists/shape_data
    cmpers -- and the internal instruction_list/data_list/shape_id references to them --
    read as `source`'s own numbering wherever a confident content-signature match exists.
    Safe to mutate rather than copy: each row is read fresh from the JSONL and used exactly
    once, by this call and the comparison right after it, never re-read afterward.

    ShapeDef, ShapeInstructionList, and ShapeData are three independent, cmper-keyed pools
    (see shape_set_font_paths()) that Finale renumbers independently on every save -- an
    instruction list, a data buffer, and the shape that ties them together can each land on
    a cmper unrelated to what the same content had in the source, and unrelated to each
    other's new numbers too (confirmed directly: of 1415 sampled shapes, 42% have a
    shapeDef/instructionList/dataList cmper triplet that is not all the same number). So
    each of the three is matched independently, by what it actually contains rather than
    its number, in dependency order: instruction lists and data buffers first (each self-
    contained), then shapes (whose signature is *built from* its own instruction list's and
    data list's signatures, not their cmpers). A companion item with no source-side
    signature match keeps its own cmper and correctly reads as reader_only/companion_only
    afterward -- the right outcome for a shape only one side actually has.

    This does not resolve every reference to a realigned cmper in the whole document, only
    the ones a survey currently emits: clef_options.clef_defs[].shape_id and
    mmrest_options.shape_def. A future surveyor field that names a shape, instruction list,
    or data list needs adding here too, the same as any other one-of-a-kind reference.
    """
    def by_signature(items: list[dict], signature_of) -> dict[Any, list[int]]:
        groups: dict[Any, list[int]] = {}
        for item in items:
            groups.setdefault(signature_of(item), []).append(item["cmper"])
        return groups

    def remap_from(source_items: list[dict], companion_items: list[dict], signature_of) -> dict[int, int]:
        available = by_signature(source_items, signature_of)
        remap: dict[int, int] = {}
        for item in companion_items:
            candidates = available.get(signature_of(item))
            if candidates:
                remap[item["cmper"]] = candidates.pop(0)
        return remap

    source_lists = (source.get("shape_instruction_lists") or {}).get("lists") or []
    companion_lists = (companion.get("shape_instruction_lists") or {}).get("lists") or []
    instruction_remap = remap_from(source_lists, companion_lists, _instruction_signature)
    source_lists_by_cmper = {lst["cmper"]: lst for lst in source_lists}
    companion_lists_by_cmper = {lst["cmper"]: lst for lst in companion_lists}
    source_shapes = source.get("shape_defs") or []
    companion_shapes = companion.get("shape_defs") or []
    source_consumed = _consumed_lengths(source_shapes, source_lists_by_cmper)
    companion_consumed = _consumed_lengths(companion_shapes, companion_lists_by_cmper)

    # Each buffer's own SetFont font-id slots are masked with that same side's own
    # positions before matching -- source's for source_data_sig, companion's for
    # companion_data_sig -- since a font id is expected to differ even between a buffer and
    # its own true match on the other side.
    source_font_positions = _shape_font_id_positions(source)
    companion_font_positions = _shape_font_id_positions(companion)
    source_buffers = (source.get("shape_data") or {}).get("buffers") or []
    companion_buffers = (companion.get("shape_data") or {}).get("buffers") or []
    source_data_sig = {b["cmper"]: _data_signature(b, source_font_positions.get(b["cmper"], set()),
        source_consumed.get(b["cmper"])) for b in source_buffers}
    companion_data_sig = {b["cmper"]: _data_signature(b, companion_font_positions.get(b["cmper"], set()),
        companion_consumed.get(b["cmper"])) for b in companion_buffers}
    available_data = by_signature(source_buffers, lambda b: source_data_sig[b["cmper"]])
    data_remap = {}
    for buffer in companion_buffers:
        candidates = available_data.get(companion_data_sig[buffer["cmper"]])
        if candidates:
            data_remap[buffer["cmper"]] = candidates.pop(0)

    def shape_signature(shape: dict, lists_by_cmper: dict[int, dict], data_sig_by_cmper: dict[int, tuple]) -> tuple:
        # shape_type deliberately does not participate: musxdom documents Other/0 as what
        # every pre-Finale-2000 shape reports (ShapeDesigner.h), since the era simply does
        # not store one, so the source side of an old shape always has it, while a
        # companion may report a specific type (Clef, say) it inferred from how the shape
        # is used rather than from anything stored. Two shapes whose instructions and data
        # are byte-identical are the same shape regardless of what each side labels it;
        # shape_type itself is still an ordinary compared field once shapes are paired.
        return (
            _instruction_signature(lists_by_cmper.get(shape.get("instruction_list"))),
            data_sig_by_cmper.get(shape.get("data_list")),
        )

    available_shapes = by_signature(source_shapes,
        lambda s: shape_signature(s, source_lists_by_cmper, source_data_sig))

    # A shape can gain a leading StartObject instruction Finale's upgrade prepends that the
    # source shape never had -- confirmed directly (MultimeasureRestOptions.shape_def:
    # source's 9 instructions unchanged, byte for byte, starting at companion's instruction
    # 1; the only difference is that leading StartObject(11) and its 11 data values). When
    # a companion shape's direct signature matches nothing, and this row is old enough that
    # a genuinely-absent shape_type is expected at all (see shape_signature()'s own note on
    # Other/0), retrying with that one leading instruction (and its data items) stripped is
    # what "same shape, Finale wrapped it" actually means -- not a value comparison to
    # excuse, a match to make. The StartObject's own data stays visible as a real, one-sided
    # addition wherever shape_instruction_lists/shape_data compare this pair leaf by leaf;
    # only the shape's own identity -- and so shape_id/shape_def references to it -- treats
    # the wrapper as not disqualifying.
    #
    # OPEN QUESTION, not yet answered: is this a real Finale-2000 boundary the way
    # shape_type is, or something narrower? checked directly against tracked-evidence
    # (2026-08-19): of the 26 shapes in the one Coda 2.6 document sampled, 12 needed this
    # stripping (including MultimeasureRestOptions.shape_def, but also 11 shapes nothing in
    # this survey references at all -- not mmrest-specific), 2 (both clef-referenced)
    # matched directly with no wrapper, and 13 matched nothing at all, wrapped or not, for
    # a reason not yet investigated. No other sampled version (1.0.0.0 through 17.0.0.146,
    # including Finale 2000's own 5.0.1.9) showed any wrapped shape. One document is not
    # enough to call this a version boundary; the private corpora (rpatters1-main/-installs)
    # have far more Coda-era and early-2000s samples and would be the next place to check.
    is_pre2k = any(s.get("origin_shapeType") == "legacy-behavior" for s in source_shapes)

    def stripped_leading_start_object(instruction_sig: tuple, data_sig: tuple | None) -> tuple[tuple, tuple] | None:
        if not instruction_sig or instruction_sig[0][0] != _START_OBJECT_INSTRUCTION_TYPE:
            return None
        num_data = instruction_sig[0][1]
        return instruction_sig[1:], (data_sig or ())[num_data:]

    shape_remap: dict[int, int] = {}
    for shape in companion_shapes:
        instruction_sig = _instruction_signature(companion_lists_by_cmper.get(shape.get("instruction_list")))
        data_sig = companion_data_sig.get(shape.get("data_list"))
        candidates = available_shapes.get((instruction_sig, data_sig))
        if not candidates and is_pre2k:
            stripped = stripped_leading_start_object(instruction_sig, data_sig)
            if stripped:
                candidates = available_shapes.get(stripped)
        if candidates:
            shape_remap[shape["cmper"]] = candidates.pop(0)

    # A genuine match's target is one of source's own cmpers -- which a companion item
    # that matched nothing is not otherwise forbidden from also already using natively.
    # Applying the three `_remap` dicts directly, as a plain overwrite, can therefore
    # collide two unrelated companion items onto the same number (confirmed directly: a
    # companion shape correctly matched to source cmper 14 collided with a companion shape
    # that already *was* cmper 14 natively, an unrelated shape). shape_final/
    # instruction_final/data_final resolve that: every companion item gets a *unique*
    # final cmper, by giving a colliding unmatched item a synthetic number instead of
    # letting it silently share one with the item that matched into its old slot.
    def safe_renumbering(companion_items: list[dict], matches: dict[int, int],
            source_cmpers: set[int]) -> dict[int, int]:
        original_cmpers = {item["cmper"] for item in companion_items}
        next_safe = (max(original_cmpers | source_cmpers) + 1) if (original_cmpers or source_cmpers) else 1
        final: dict[int, int] = {}
        for item in companion_items:
            cmper = item["cmper"]
            if cmper in matches:
                final[cmper] = matches[cmper]
            elif cmper in source_cmpers:
                final[cmper] = next_safe
                next_safe += 1
            else:
                final[cmper] = cmper
        return final

    shape_final = safe_renumbering(companion_shapes, shape_remap,
        {s["cmper"] for s in source_shapes})
    instruction_final = safe_renumbering(companion_lists, instruction_remap,
        {lst["cmper"] for lst in source_lists})
    data_final = safe_renumbering(companion_buffers, data_remap,
        {b["cmper"] for b in source_buffers})

    for shape in companion_shapes:
        shape["cmper"] = shape_final[shape["cmper"]]
        shape["instruction_list"] = instruction_final.get(
            shape["instruction_list"], shape["instruction_list"])
        shape["data_list"] = data_final.get(shape["data_list"], shape["data_list"])
    for instruction_list in companion_lists:
        instruction_list["cmper"] = instruction_final[instruction_list["cmper"]]
    for buffer in companion_buffers:
        buffer["cmper"] = data_final[buffer["cmper"]]
    for clef in ((companion.get("clef_options") or {}).get("clef_defs") or []):
        clef["shape_id"] = shape_final.get(clef["shape_id"], clef["shape_id"])
    mmrest = companion.get("mmrest_options")
    if mmrest and "shape_def" in mmrest:
        mmrest["shape_def"] = shape_final.get(mmrest["shape_def"], mmrest["shape_def"])


def _font_definition_key(item: dict) -> tuple[str, str] | None:
    """font_definitions.definitions is keyed by name, not cmper: Finale reassigns a font
    definition's cmper on every save, in that save's own reference order, so a nonzero one
    carries no cross-document identity at all -- cmper 4 can be "Monaco" in one document's
    font table and "Times New Roman" in another's. Comparator 0 is the one exception:
    both musxdom and every source agree it names the document's default music font
    specifically by that id (see text_encoding.h), never by whatever name a resave gives
    it, so 0 is matched on cmper and everything else on normalized_name.
    """
    if not isinstance(item, dict):
        return None
    if item.get("cmper") == 0:
        return ("cmper", "0")
    if "normalized_name" in item:
        return ("normalized_name", str(item["normalized_name"]))
    return None


# list_path (the dotted path to the *list itself*, e.g. "font_definitions.definitions") ->
# a function from one item to its (field, key) match identity, overriding the default
# cmper-based match below for classes whose cmper is not a stable cross-document identity.
LIST_MATCH_KEY_OVERRIDES: dict[str, Any] = {
    "font_definitions.definitions": _font_definition_key,
}


def list_path_segments(items: list, list_path: str) -> list[str]:
    """The path segment for each item in `items` (the list found at `list_path`): keyed by
    `LIST_MATCH_KEY_OVERRIDES[list_path]` when that applies to every item, else by `cmper`
    when every item is a dict carrying one (a stable identifier Finale itself assigns for
    most classes -- see e.g. surveyors/others/shape_definitions.cpp), positional index
    otherwise. Keying by identity rather than position is what lets a source list and a
    companion list align correctly: neither side is guaranteed to write records in the same
    order or count, and the reader recovering one more or fewer than the companion should
    not misalign every record after it the way positional indexing would. A repeated key
    within one list is suffixed `#2`, `#3`, ... in encounter order rather than silently
    collapsed onto the first, so two same-named fonts (say) still each get their own path --
    imperfectly, since encounter order is the only thing pairing them, but not silently.
    """
    override = LIST_MATCH_KEY_OVERRIDES.get(list_path)
    keys: list[tuple[str, str]] | None = None
    if override is not None:
        resolved = [override(item) for item in items]
        if items and all(key is not None for key in resolved):
            keys = resolved
    if keys is None:
        if items and all(isinstance(item, dict) and "cmper" in item for item in items):
            keys = [("cmper", str(item["cmper"])) for item in items]
        else:
            return [f"[{index}]" for index in range(len(items))]
    seen: Counter[tuple[str, str]] = Counter()
    segments = []
    for field, key in keys:
        seen[(field, key)] += 1
        suffix = "" if seen[(field, key)] == 1 else f"#{seen[(field, key)]}"
        segments.append(f"[{field}={key}{suffix}]")
    return segments


def snake_to_camel(name: str) -> str:
    first, *rest = name.split("_")
    return first + "".join(word.capitalize() for word in rest)


def leaf_paths(value: Any, prefix: str = "", origin: str | None = None) -> Iterator[tuple[str, Any, str | None]]:
    """Yields (dotted/bracketed path, leaf value, origin) for every leaf under `value`,
    skipping metadata and source-only keys (see is_source_only_key()) at any depth.

    `origin` is that leaf's own `origin_<fieldName>` (or font_options.cpp's
    `<fieldName>_origin`) sibling value, when the object one level up carries one -- None
    for a companion, which has no such concept, and for a leaf with no origin sibling.
    It is not itself a value to diff (see is_source_only_key()) but a rule in
    EXPECTED_DIFFERENCES may still condition on it: a diff is a different claim depending
    on whether the source side actually recovered the value or is reporting a reference's
    own pinned-baseline default (`origin == "finale27-default"`), which was never going to
    match a specific companion's own numbering for that reference in the first place.
    """
    if isinstance(value, dict):
        for key, sub in value.items():
            if key in METADATA_KEYS or is_source_only_key(key):
                continue
            child_prefix = f"{prefix}.{key}" if prefix else key
            child_origin = value.get(f"origin_{snake_to_camel(key)}") or value.get(f"{key}_origin")
            yield from leaf_paths(sub, child_prefix, child_origin)
    elif isinstance(value, list):
        for segment, item in zip(list_path_segments(value, prefix), value):
            yield from leaf_paths(item, f"{prefix}{segment}")
    else:
        yield prefix, value, origin


def classify_difference(path: str, category: Category, source_origin: str | None) -> str | None:
    """The label for an expected difference at `path` in `category`, or None when it is
    unexpected. `source_origin` is that leaf's origin annotation on the source side (see
    leaf_paths()), checked against a rule's `origins` when it specifies one.
    """
    for pattern, categories, origins, label in EXPECTED_DIFFERENCES:
        if categories is not None and category not in categories:
            continue
        if origins is not None and source_origin not in origins:
            continue
        if re.search(pattern, path):
            return label
    return None


class ClassStats:
    def __init__(self) -> None:
        self.same = 0
        self.expected_diff = 0
        self.unexpected_diff = 0
        self.reader_only = 0
        self.companion_only = 0

    def total(self) -> int:
        return (self.same + self.expected_diff + self.unexpected_diff
                + self.reader_only + self.companion_only)


def compare_row(source: dict, companion: dict) -> tuple[
        dict[str, ClassStats], list[tuple[str, str, Any, Any]], list[tuple[str, str, str]]]:
    """Compares every surveyed class present in either `source` or `companion`.

    Returns per-class ClassStats; the list of (class, path, source_value, companion_value)
    leaves classified as unexpected -- differing, reader-only, or companion-only with no
    matching rule in EXPECTED_DIFFERENCES; and the list of (path, source_font_name,
    companion_font_name) SetFont substitutions found and excused (see
    shape_set_font_paths()) -- excused from the regression count, but still surfaced by
    name rather than silently swallowed as a raw-number mismatch, since a font id
    disagreeing is expected but *which* font it substituted is still worth a look.
    """
    stats: dict[str, ClassStats] = {}
    unexpected: list[tuple[str, str, Any, Any]] = []
    substitutions: list[tuple[str, str, str]] = []
    # Must run before shape_set_font_paths(source) is used below and before any
    # leaf_paths() call over companion's shape_defs/shape_instruction_lists/shape_data:
    # it mutates companion's own cmpers in place to match source's numbering.
    realign_shape_cmpers(source, companion)
    source_font_id_paths = shape_set_font_paths(source)
    classes = (set(source) | set(companion)) - METADATA_KEYS
    for class_name in classes:
        class_stats = stats.setdefault(class_name, ClassStats())
        # Seeded with class_name, not "", so every path leaf_paths() yields (and every
        # LIST_MATCH_KEY_OVERRIDES lookup inside it) is already fully qualified -- matching
        # what full_path needs below without a second, easy-to-desync prefixing step here.
        # {path: (value, source_origin)}; companion_leaves' origin slot is always None
        # (see leaf_paths()) and is not read below.
        source_leaves = {path: (value, origin)
            for path, value, origin in leaf_paths(source.get(class_name), class_name)}
        companion_leaves = {path: (value, origin)
            for path, value, origin in leaf_paths(companion.get(class_name), class_name)}
        for full_path in set(source_leaves) | set(companion_leaves):
            in_source = full_path in source_leaves
            in_companion = full_path in companion_leaves
            if in_source and in_companion:
                if source_leaves[full_path][0] == companion_leaves[full_path][0]:
                    class_stats.same += 1
                    continue
                category = "differs"
            elif in_source:
                category = "reader_only"
            else:
                category = "companion_only"

            if category == "differs" and full_path in source_font_id_paths:
                class_stats.expected_diff += 1
                source_name = resolve_font_name(source, source_leaves[full_path][0])
                companion_name = resolve_font_name(companion, companion_leaves[full_path][0])
                substitutions.append((full_path, source_name or "?", companion_name or "?"))
                continue

            source_origin = source_leaves[full_path][1] if in_source else None
            label = classify_difference(full_path, category, source_origin)
            if label is not None:
                class_stats.expected_diff += 1
                continue

            if category == "differs":
                class_stats.unexpected_diff += 1
            elif category == "reader_only":
                class_stats.reader_only += 1
            else:
                class_stats.companion_only += 1
            source_value = source_leaves[full_path][0] if in_source else None
            companion_value = companion_leaves[full_path][0] if in_companion else None
            unexpected.append((class_name, full_path, source_value, companion_value))
    return stats, unexpected, substitutions


def read_rows(path: Path) -> list[dict]:
    rows = []
    with path.open(encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            rows.append(json.loads(line))
    return rows


def print_table(title: str, headers: list[str], rows: Iterable[list[str]]) -> None:
    rows = list(rows)
    print(f"\n{title}")
    print("=" * len(title))
    if not rows:
        print("(none)")
        return
    widths = [len(h) for h in headers]
    for row in rows:
        for index, cell in enumerate(row):
            widths[index] = max(widths[index], len(cell))
    def line(cells: list[str]) -> str:
        return "  ".join(cell.ljust(widths[index]) for index, cell in enumerate(cells))
    print(line(headers))
    print("  ".join("-" * width for width in widths))
    for row in rows:
        print(line(row))


def report_import_summary(rows: list[dict]) -> None:
    status_counts = Counter(row.get("status", "?") for row in rows)
    epoch_counts = Counter(
        row.get("epoch", "-") for row in rows if row.get("status") == "ok")
    print(f"\n{len(rows)} document(s): "
        f"{status_counts.get('ok', 0)} ok, {status_counts.get('error', 0)} error")
    print_table("Epoch (ok documents)", ["epoch", "count"],
        ([epoch, str(count)] for epoch, count in epoch_counts.most_common()))


def report_diagnostics(rows: list[dict], from_companion: bool, title: str) -> None:
    counts: Counter[tuple[str, str]] = Counter()
    examples: dict[tuple[str, str], str] = {}
    for row in rows:
        container = row.get("companion") if from_companion else row
        if container is None:
            continue
        diagnostics = container.get("diagnostics", [])
        for diagnostic in diagnostics:
            entry = (diagnostic.get("level", "?"), diagnostic.get("message", ""))
            counts[entry] += 1
            examples.setdefault(entry, row.get("corpus_id", "?"))
    print_table(title, ["level", "count", "example", "message"],
        ([level, str(count), examples[(level, message)], message]
            for (level, message), count in counts.most_common()))


def report_failures(rows: list[dict]) -> None:
    counts: Counter[str] = Counter()
    examples: dict[str, str] = {}
    for row in rows:
        if row.get("status") != "error":
            continue
        message = row.get("error", "")
        counts[message] += 1
        examples.setdefault(message, row.get("corpus_id", "?"))
    print_table("Failure reasons", ["count", "example", "message"],
        ([str(count), examples[message], message] for message, count in counts.most_common()))


def report_companion_comparison(rows: list[dict], max_unexpected: int) -> None:
    companion_rows = [row for row in rows if row.get("companion")]
    if not companion_rows:
        print("\nNo rows carried a companion (corpus declared no #companion: convention, "
            "or every source failed to import).")
        return

    companion_status = Counter(row["companion"].get("status", "?") for row in companion_rows)
    print(f"\n{len(companion_rows)} row(s) with a companion: "
        f"{companion_status.get('ok', 0)} ok, {companion_status.get('error', 0)} error")

    totals: dict[str, ClassStats] = {}
    all_unexpected: list[tuple[str, str, str, Any, Any]] = []
    font_substitutions: Counter[tuple[str, str]] = Counter()
    for row in companion_rows:
        companion = row["companion"]
        if companion.get("status") != "ok":
            continue
        stats, unexpected, substitutions = compare_row(row, companion)
        for class_name, class_stats in stats.items():
            total = totals.setdefault(class_name, ClassStats())
            total.same += class_stats.same
            total.expected_diff += class_stats.expected_diff
            total.unexpected_diff += class_stats.unexpected_diff
            total.reader_only += class_stats.reader_only
            total.companion_only += class_stats.companion_only
        for class_name, path, source_value, companion_value in unexpected:
            all_unexpected.append(
                (row.get("corpus_id", "?"), class_name, path, source_value, companion_value))
        for _path, source_name, companion_name in substitutions:
            font_substitutions[(source_name, companion_name)] += 1

    print_table("SetFont substitutions (excused from the counts above)",
        ["source font", "companion font", "count"],
        ([source_name, companion_name, str(count)]
            for (source_name, companion_name), count in font_substitutions.most_common()))

    print_table("Companion comparison (leaves), by class",
        ["class", "same", "expected-diff", "unexpected-diff", "reader-only", "companion-only", "total"],
        ([name, str(s.same), str(s.expected_diff), str(s.unexpected_diff),
            str(s.reader_only), str(s.companion_only), str(s.total())]
            for name, s in sorted(totals.items(), key=lambda item: -item[1].total())))

    if all_unexpected:
        shown = all_unexpected[:max_unexpected]
        print_table(
            f"Unexpected differences (first {len(shown)} of {len(all_unexpected)})",
            ["corpus_id", "path", "source", "companion"],
            ([corpus_id, full_path, json.dumps(source_value), json.dumps(companion_value)]
                for corpus_id, _class_name, full_path, source_value, companion_value in shown))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("jsonl", type=Path, help="recovery_coverage_probe output")
    parser.add_argument("--max-unexpected", type=int, default=40,
        help="cap on unexpected-difference rows printed (default: 40)")
    args = parser.parse_args()

    rows = read_rows(args.jsonl)
    if not rows:
        print("no rows in input", file=sys.stderr)
        return 1

    report_import_summary(rows)
    report_diagnostics(rows, False, "Source diagnostics")
    report_failures(rows)
    report_diagnostics(rows, True, "Companion diagnostics")
    report_companion_comparison(rows, args.max_unexpected)
    return 0


if __name__ == "__main__":
    sys.exit(main())
