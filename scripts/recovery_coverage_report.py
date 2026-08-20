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
from typing import Any, Callable, Iterable, Iterator, Optional

# Row keys that describe the row/import itself rather than surveyed content. Excluded from
# both the per-class breakdown and the recursive companion diff.
METADATA_KEYS = {
    "corpus_id", "status", "epoch", "saving_product", "source_version",
    "warning_count", "diagnostics", "duration_ms", "companion", "finder_type", "error",
}

# Surveyors retained in the JSONL for recovery exploration but not mature enough for
# companion-quality claims yet. Excluding them here avoids presenting proof-of-concept
# fields as reader regressions while preserving the snapshot for later analysis.
COMPARISON_EXCLUDED_CLASSES = {
    "layer_atts",
    "spacing_options",
}

# The probe's surveyor directories define pool ownership; keep the same names here so the
# report presents separate DOM pools rather than one alphabetized but semantically mixed
# list. New surveyors intentionally fall into `unclassified` until their pool is stated --
# disappearing from the report would be much worse than an obvious missing classification.
SURVEY_CLASS_POOLS = {
    "block_texts": "texts",
    "bookmark_texts": "texts",
    "clef_options": "options",
    "expression_texts": "texts",
    "file_info_texts": "texts",
    "font_definitions": "others",
    "font_options": "options",
    "layer_atts": "others",
    "lyric_options": "options",
    "lyrics_choruses": "texts",
    "lyrics_sections": "texts",
    "lyrics_verses": "texts",
    "meas_graphic_assigns": "details",
    "mmrest_options": "options",
    "page_graphic_assigns": "others",
    "shape_data": "others",
    "shape_defs": "others",
    "shape_graphic_assigns": "others",
    "shape_instruction_lists": "others",
    "smart_shape_texts": "texts",
    "spacing_options": "options",
    "ss_line_styles": "others",
    "stem_options": "options",
    "text_options": "options",
}


def is_noncontent_key(key: str) -> bool:
    """True when a key has the same non-content meaning wherever it appears.

    Field origins and legacy byte offsets are recovery provenance. Indices and cmpers
    identify leaves; counting them again as values only multiplies a one-sided object
    difference. Aggregate instruction/data counts duplicate the collections themselves.
    All remain useful in the JSONL but none participates in companion comparison.
    """
    return (key in {"origin", "index", "cmper", "instruction_count", "value_count",
            "external_graphic_count", "undocumented_instruction_count"}
        or key.startswith("origin_") or key.endswith("_origin")
        or key.endswith("_block_offset") or key.endswith("_decoded_field_offset"))


# Fully qualified fields or subtrees that are useful recovery diagnostics but are not
# companion-comparable document content. Unlike is_noncontent_key(), these names have that
# meaning only at their stated path. A path excludes itself and everything nested below it.
COMPARISON_EXCLUDED_PATHS = {
    "font_options.tuples",
    "font_options.recovered_count",
    "font_options.legacy_behavior_count",
    "font_options.default_count",
    "font_definitions.duplicate_nonzero_name_count",
    "font_definitions.introduced_duplicate_nonzero_name_count",
    # This histogram summarizes the instruction leaves compared under `lists`; comparing
    # it again would turn every one-sided list or normalized wrapper into a second diff.
    "shape_instruction_lists.instruction_types",
}

# Patterned exclusions whose dynamic list identity prevents spelling them as exact paths.
# FontDefinition arrays are keyed and compared by normalized_name; comparing their raw name
# as well would count spelling-only differences a second time.
COMPARISON_EXCLUDED_PATH_PATTERNS = [
    re.compile(r"^font_definitions\.definitions\[[^\]]+\]\.name$"),
]

# A legacy source names a font in its Enigma string, while Finale may rewrite that same
# selection as a document-local font id or category. The retired options report normalized
# these equivalent spellings before comparing text; everything outside the command remains
# subject to exact comparison here.
ENIGMA_FONT_COMMAND = re.compile(
    r"\^(?:font|fontid|Font|fontMus|fontTxt|fontNum)\([^)]*\)")
ENIGMA_TIME_INSERT = re.compile(r"\^time\([^)]*\)")
ENIGMA_COMPLETE_FONT_STATE = re.compile(
    r"(\^(?:font|fontid|Font|fontMus|fontTxt|fontNum)\([^)]*\)"
    r"\^size\([^)]*\)\^nfx\([^)]*\))(?:\1)+")

# Finale does not use font information on these two text classes, and its companions may
# therefore omit the entire initial state. The reader emits a complete state so the Enigma
# string is independently valid. Keep this narrower than general text normalization: a
# companion that names any font, or a source prefix that is not exactly the reader's
# face/size/nfx completion, remains comparable content.
NONDISPLAY_TEXT_CLASSES = {"bookmark_texts", "file_info_texts"}
COMPLETE_INITIAL_FONT_STATE = re.compile(
    r"^\^(?:font|fontid|Font|fontMus|fontTxt|fontNum)\([^)]*\)"
    r"\^size\([^)]*\)\^nfx\([^)]*\)")


def normalize_enigma_font_commands(value: Any) -> Any:
    return ENIGMA_FONT_COMMAND.sub("<F>", value) if isinstance(value, str) else value


def normalize_enigma_time_inserts(value: Any) -> Any:
    """Removes the time insert that Finale drops while upgrading legacy text."""
    return ENIGMA_TIME_INSERT.sub("", value) if isinstance(value, str) else value


def normalize_duplicate_enigma_font_states(value: Any) -> Any:
    """Collapses exact adjacent complete states that Finale removes on upgrade."""
    return ENIGMA_COMPLETE_FONT_STATE.sub(r"\1", value) if isinstance(value, str) else value


def equal_when_companion_omits_nondisplay_font_state(
        class_name: str, source_value: Any, companion_value: Any) -> bool:
    """Treats the reader's validity prefix as absent when Finale omitted it."""
    if (class_name not in NONDISPLAY_TEXT_CLASSES
            or not isinstance(source_value, str) or not isinstance(companion_value, str)
            or ENIGMA_FONT_COMMAND.search(companion_value)):
        return False
    return COMPLETE_INITIAL_FONT_STATE.sub("", source_value, count=1) == companion_value


def is_comparison_excluded_path(path: str) -> bool:
    return (any(path == excluded or path.startswith(excluded + ".")
        or path.startswith(excluded + "[") for excluded in COMPARISON_EXCLUDED_PATHS)
        or any(pattern.search(path) for pattern in COMPARISON_EXCLUDED_PATH_PATTERNS))


Category = str  # "differs" | "reader_only" | "companion_only" ("same" is never classified)

# (path_regex, categories, origins, value_condition, label): a leaf whose dotted path matches, whose
# category is in `categories` (None means any), and whose source-side origin annotation is
# in `origins` (None means don't care), and whose value condition accepts it (None means
# don't care) is an intended difference or a structurally
# one-sided value, not a regression candidate. Seeded from real unexpected-difference runs
# against tracked-evidence and exceptions carried forward from the retired options comparator.
# Scoping by category and origin
# matters: a leaf that is reader_only or companion_only because one side's collection is
# longer than the other is a different claim from the same path *differing* where both
# sides have a value, and a rule should not paper over the second just because the first is
# expected; likewise a value this reader actually recovered from the source disagreeing
# with the companion is a different claim from a synthesized reference default disagreeing
# with it, even at the same path.
ValueCondition = Callable[[str, Any, Any, dict[str, tuple[Any, Optional[str]]],
    dict[str, tuple[Any, Optional[str]]], dict[str, Any]], bool]


def is_finale_symbol_charset_normalization(path: str, source_value: Any,
        companion_value: Any, source_leaves: dict[str, tuple[Any, str | None]],
        companion_leaves: dict[str, tuple[Any, str | None]], _source_row: dict[str, Any]) -> bool:
    """Finale changed charsetVal 0 to the Mac symbol value while retaining bank 0.

    The definition list is already aligned by normalized font name, so both leaves belong
    to the same named font. No font-name or version heuristic participates in the rule.
    """
    if source_value != 0 or companion_value != 4095:
        return False
    bank_path = path.removesuffix(".charset_value") + ".charset_bank"
    return (source_leaves.get(bank_path, (None, None))[0] == 0
        and companion_leaves.get(bank_path, (None, None))[0] == 0)


def is_false_to_true(_path: str, source_value: Any, companion_value: Any,
        _source_leaves: dict[str, tuple[Any, Optional[str]]],
        _companion_leaves: dict[str, tuple[Any, Optional[str]]],
        _source_row: dict[str, Any]) -> bool:
    return source_value is False and companion_value is True


def is_absent_legacy_insert_default(path: str, source_value: Any, companion_value: Any,
        _source_leaves: dict[str, tuple[Any, Optional[str]]],
        _companion_leaves: dict[str, tuple[Any, Optional[str]]],
        _source_row: dict[str, Any]) -> bool:
    """The two exact insert defaults Finale synthesizes differently from our baseline."""
    return ((path == "text_options.inserts[1].tracking_before"
                and source_value == 60 and companion_value == 50)
        or (path == "text_options.inserts[2].tracking_before"
                and source_value == 50 and companion_value == 0))


def is_finale_17_byte_insert_misconversion(_path: str, _source_value: Any,
        _companion_value: Any, source_leaves: dict[str, tuple[Any, Optional[str]]],
        companion_leaves: dict[str, tuple[Any, Optional[str]]],
        _source_row: dict[str, Any]) -> bool:
    """Finale's complete, recognizable mis-conversion of the 17-byte insert layout.

    The five tracking values identify the record's structure without relying on a recovered
    marketing version. Requiring the entire source and companion signatures prevents one
    coincidentally equal shifted value from excusing an unrelated insert-field difference.
    """
    source_signature = (35, 50, 0, 40, 60)
    companion_signature = (2293760, 587202560, 0, 1845493760, 3932160)

    def tracking_signature(leaves: dict[str, tuple[Any, Optional[str]]]) -> tuple[Any, ...]:
        return tuple(leaves.get(
            f"text_options.inserts[{index}].tracking_before", (None, None))[0]
            for index in range(5))

    return (tracking_signature(source_leaves) == source_signature
        and tracking_signature(companion_leaves) == companion_signature)


def is_coda_default_stem_horizontal_correction(_path: str, source_value: Any,
        companion_value: Any, source_leaves: dict[str, tuple[Any, Optional[str]]],
        companion_leaves: dict[str, tuple[Any, Optional[str]]],
        source_row: dict[str, Any]) -> bool:
    """Finale's font-dependent horizontal correction of the Coda default stem glyph.

    All other fields of the first connection must survive exactly. That both limits the
    exception to the observed one-field transformation and prevents a damaged or misaligned
    connection from being excused merely because its horizontal value happens to match one
    of the three observed companion values.
    """
    if (source_row.get("epoch") != "coda-banner" or source_value != 0
            or companion_value not in {221, 589, 6969}):
        return False
    prefix = "stem_options.stem_connections[0]."
    unchanged_fields = (
        "font_name", "font_id", "symbol", "up_stem_vert", "down_stem_vert",
        "down_stem_horz",
    )
    return all(source_leaves.get(prefix + field, (None, None))[0]
        == companion_leaves.get(prefix + field, (None, None))[0]
        for field in unchanged_fields)


def is_coda_synthesized_stem_width(_path: str, source_value: Any,
        companion_value: Any, _source_leaves: dict[str, tuple[Any, Optional[str]]],
        _companion_leaves: dict[str, tuple[Any, Optional[str]]],
        source_row: dict[str, Any]) -> bool:
    """Finale synthesizes a Coda stem width where the source stores no such option.

    The reader's 115 is the pinned baseline, not a recovered Coda value. Finale's upgrader
    independently chooses 224 for the observed Finale 1.0 files and 128 for Finale 2.6;
    neither value is evidence of a legacy field that the reader should recover.
    """
    return (source_row.get("epoch") == "coda-banner" and source_value == 115
        and companion_value in {128, 224})


def is_coda_synthesized_stem_offset(_path: str, source_value: Any,
        companion_value: Any, _source_leaves: dict[str, tuple[Any, Optional[str]]],
        _companion_leaves: dict[str, tuple[Any, Optional[str]]],
        source_row: dict[str, Any]) -> bool:
    """Finale synthesizes a Coda stem offset where the source stores no such option."""
    return (source_row.get("epoch") == "coda-banner" and source_value == 256
        and companion_value == 128)


def is_pre_connection_table_one_entry_offset(path: str, source_value: Any,
        companion_value: Any, source_leaves: dict[str, tuple[Any, Optional[str]]],
        companion_leaves: dict[str, tuple[Any, Optional[str]]],
        source_row: dict[str, Any]) -> bool:
    """Finale changes the baseline one-entry endpoint before the table was stored."""
    if source_value != 42 or companion_value != 44:
        return False
    try:
        before_connection_table = int(str(source_row.get("source_version", "")).split('.')[0]) < 9
    except ValueError:
        before_connection_table = False
    if source_row.get("epoch") == "coda-banner":
        before_connection_table = True
    if not before_connection_table:
        return False
    prefix = "lyric_options.word_ext_connect_styles."
    surrounding = {leaf for leaf in set(source_leaves) | set(companion_leaves)
        if leaf.startswith(prefix) and leaf != path}
    return all(source_leaves.get(leaf, (None, None))[0]
            == companion_leaves.get(leaf, (None, None))[0]
        for leaf in surrounding)


EXPECTED_DIFFERENCES: list[tuple[
        str, set[Category] | None, set[str | None] | None, ValueCondition | None, str]] = [
    (r"^stem_options\.stem_connections\[", {"companion_only"}, None, None,
        "past-terminator: musxdom completes the stem-connection table past the first "
        "symbol-less entry, where this reader stops (see stem_options.cpp)"),
    (r"^stem_options\.stem_connections\[0\]\.up_stem_horz$",
        {"differs"}, {"legacy-mus"}, is_coda_default_stem_horizontal_correction,
        "Finale applied one of the observed font-dependent horizontal corrections to the "
        "Coda-era default stem glyph while leaving every other connection field unchanged"),
    (r"^stem_options\.stem_width$", {"differs"}, {"finale27-default"},
        is_coda_synthesized_stem_width,
        "the Coda source stores no stem-width option: this reader retains the pinned "
        "baseline's 115 while Finale's upgrader synthesizes the observed 224 or 128"),
    (r"^stem_options\.stem_offset$", {"differs"}, {"finale27-default"},
        is_coda_synthesized_stem_offset,
        "the Coda source stores no stem-offset option: this reader retains the pinned "
        "baseline's 256 while Finale's upgrader synthesizes 128"),
    (r"\bshape_id$", {"differs"}, {"finale27-default"}, None,
        "a shape_id the source side never recovered -- it is the pinned Finale 27 "
        "baseline's own default reference -- so it names a shape in that baseline's own "
        "numbering, not this specific companion's; shape ids are reassigned per save (see "
        "the cmper instability note on LIST_MATCH_KEY_OVERRIDES), so the two numbers "
        "agreeing was never the right test. What should agree, and isn't checked here yet, "
        "is whether the two referenced shapes are the same shape."),
    (r"^font_definitions\.definitions\[(?:normalized_name=[^\]]+|cmper=0)\]\.charset_value$",
        {"differs"}, None, is_finale_symbol_charset_normalization,
        "Finale normalized charsetVal 0 to the Mac symbol value 4095 on the same "
        "normalized font definition while retaining charsetBank 0"),
    (r"^lyric_options\.(?:use_smart_hyphens|use_smart_word_extensions)$",
        {"differs"}, {"legacy-behavior"}, is_false_to_true,
        "Finale enabled smart lyric hyphens and word extensions while upgrading the "
        "lyrics and synthesizing the smart shapes that implement them; the source era "
        "had no stored option, so this does not excuse a later explicit false value"),
    (r"^text_options\.inserts\[(?:1|2)\]\.tracking_before$",
        {"differs"}, {"finale27-default"}, is_absent_legacy_insert_default,
        "the source has no accidental-insert record: this reader retains the pinned "
        "Finale 27 baseline while Finale's upgrader synthesizes the older flat and "
        "natural tracking defaults"),
    (r"^lyric_options\.word_ext_connect_styles\.oneEntryEnd\.x$",
        {"differs"}, None, is_pre_connection_table_one_entry_offset,
        "before the connection table exists, Finale changes the baseline one-entry "
        "endpoint from 42 to 44 while leaving every other connection-style field intact; "
        "the reader keeps the pinned baseline rather than guessing the upgrade formula"),
    (r"^text_options\.inserts\[\d+\]\.", {"differs"}, None,
        is_finale_17_byte_insert_misconversion,
        "Finale mis-converted the complete 17-byte accidental-insert layout as the later "
        "18-byte layout; the five tracking values structurally identify the transformation"),
]

# ShapeDefInstructionType::SetFont's ordinal (see musxdom's ShapeDesigner.h); its first of
# three data items is a font id (font id, size, efx). No other shape instruction references
# a font.
_SET_FONT_INSTRUCTION_TYPE = 20

# ShapeDefInstructionType::StartGroup's ordinal. A source shape beginning with either
# boundary is outside the observed boundaryless-shape upgrade case below.
_START_GROUP_INSTRUCTION_TYPE = 24

# ShapeDefInstructionType::StartObject's ordinal -- the instruction realign_shape_cmpers()
# recognizes as a companion-added wrapper when the remainder is an exact shape match.
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


def realign_shape_cmpers(source: dict, companion: dict) -> int:
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

    Returns the number of structurally proven Finale-added StartObject wrappers removed
    while aligning the pools. They are reported as transformations, not leaf differences:
    removing the wrapper prevents its positional insertion from making every unchanged
    tail leaf appear different.

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

    # Some Finale upgrades prepend StartObject and its data to a shape that stored neither
    # StartObject nor StartGroup. This is not version-gated: corpus evidence includes both
    # wrapped and unwrapped boundaryless shapes within Finale 3.7 and Finale 98. The exact
    # remainder is the structural marker. It recognizes what Finale did in the companion;
    # it does not imply that the reader should synthesize the instruction.

    def stripped_leading_start_object(instruction_sig: tuple, data_sig: tuple | None) -> tuple[tuple, tuple] | None:
        if not instruction_sig or instruction_sig[0][0] != _START_OBJECT_INSTRUCTION_TYPE:
            return None
        num_data = instruction_sig[0][1]
        return instruction_sig[1:], (data_sig or ())[num_data:]

    shape_remap: dict[int, int] = {}
    wrapped_matches: list[tuple[dict, dict, int]] = []
    source_shapes_by_cmper = {shape["cmper"]: shape for shape in source_shapes}
    for shape in companion_shapes:
        instruction_sig = _instruction_signature(companion_lists_by_cmper.get(shape.get("instruction_list")))
        data_sig = companion_data_sig.get(shape.get("data_list"))
        candidates = available_shapes.get((instruction_sig, data_sig))
        wrapped_num_data = 0
        if not candidates:
            stripped = stripped_leading_start_object(instruction_sig, data_sig)
            source_starts_with_boundary = (stripped and stripped[0]
                and stripped[0][0][0] in {
                    _START_GROUP_INSTRUCTION_TYPE, _START_OBJECT_INSTRUCTION_TYPE})
            if stripped and not source_starts_with_boundary:
                candidates = available_shapes.get(stripped)
                if candidates:
                    wrapped_num_data = instruction_sig[0][1]
        if candidates:
            source_cmper = candidates.pop(0)
            shape_remap[shape["cmper"]] = source_cmper
            if wrapped_num_data:
                wrapped_matches.append((shape, source_shapes_by_cmper[source_cmper], wrapped_num_data))

    # A wrapper match also establishes the identity of its instruction and data pools.
    # Align those pools, remove only the proven prefix, and restore positional indices so
    # the unchanged remainder receives the ordinary leaf-by-leaf comparison.
    wrapper_count = 0
    stripped_instruction_lists: set[int] = set()
    stripped_data_lists: set[int] = set()
    companion_buffers_by_cmper = {buffer["cmper"]: buffer for buffer in companion_buffers}
    for companion_shape, source_shape, num_data in wrapped_matches:
        companion_instruction_cmper = companion_shape["instruction_list"]
        source_instruction_cmper = source_shape["instruction_list"]
        instruction_remap[companion_instruction_cmper] = source_instruction_cmper
        if companion_instruction_cmper not in stripped_instruction_lists:
            instruction_list = companion_lists_by_cmper[companion_instruction_cmper]
            instruction_list["instructions"].pop(0)
            for index, instruction in enumerate(instruction_list["instructions"]):
                instruction["index"] = index
            stripped_instruction_lists.add(companion_instruction_cmper)
            wrapper_count += 1

        companion_data_cmper = companion_shape["data_list"]
        source_data_cmper = source_shape["data_list"]
        data_remap[companion_data_cmper] = source_data_cmper
        if companion_data_cmper not in stripped_data_lists:
            buffer = companion_buffers_by_cmper[companion_data_cmper]
            del buffer["values"][:num_data]
            for index, value in enumerate(buffer["values"]):
                value["index"] = index
            stripped_data_lists.add(companion_data_cmper)

    # Shape-data values beyond the number consumed by the owning instruction stream are
    # unused padding. They participate neither in shape matching nor in the semantic diff.
    def trim_padding(buffers: list[dict], consumed_lengths: dict[int, int]) -> None:
        for buffer in buffers:
            consumed = consumed_lengths.get(buffer["cmper"])
            if consumed is not None:
                del buffer["values"][consumed:]

    trim_padding(source_buffers, source_consumed)
    trim_padding(companion_buffers,
        _consumed_lengths(companion_shapes, companion_lists_by_cmper))

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
    return wrapper_count


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


def _text_record_key(item: dict) -> tuple[str, str] | None:
    """A text record's stable pool identity, independent of array serialization order."""
    if isinstance(item, dict) and "number" in item:
        return ("number", str(item["number"]))
    return None


# list_path (the dotted path to the *list itself*, e.g. "font_definitions.definitions") ->
# a function from one item to its (field, key) match identity, overriding the default
# cmper-based match below for classes whose cmper is not a stable cross-document identity.
LIST_MATCH_KEY_OVERRIDES: dict[str, Any] = {
    "font_definitions.definitions": _font_definition_key,
    **{class_name: _text_record_key for class_name, pool in SURVEY_CLASS_POOLS.items()
        if pool == "texts"},
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
    skipping metadata, globally non-content keys, and path-specific recovery diagnostics
    (see is_noncontent_key() and COMPARISON_EXCLUDED_PATHS) at any depth.

    `origin` is that leaf's own `origin_<fieldName>` (or font_options.cpp's
    `<fieldName>_origin`) sibling value, when the object one level up carries one -- None
    for a companion, which has no such concept, and for a leaf with no origin sibling.
    It is not itself a value to diff (see is_noncontent_key()) but a rule in
    EXPECTED_DIFFERENCES may still condition on it: a diff is a different claim depending
    on whether the source side actually recovered the value or is reporting a reference's
    own pinned-baseline default (`origin == "finale27-default"`), which was never going to
    match a specific companion's own numbering for that reference in the first place.
    """
    if isinstance(value, dict):
        for key, sub in value.items():
            if key in METADATA_KEYS or is_noncontent_key(key):
                continue
            child_prefix = f"{prefix}.{key}" if prefix else key
            if is_comparison_excluded_path(child_prefix):
                continue
            child_origin = value.get(f"origin_{snake_to_camel(key)}") or value.get(f"{key}_origin")
            yield from leaf_paths(sub, child_prefix, child_origin)
    elif isinstance(value, list):
        for segment, item in zip(list_path_segments(value, prefix), value):
            yield from leaf_paths(item, f"{prefix}{segment}")
    else:
        yield prefix, value, origin


def classify_difference(path: str, category: Category, source_origin: str | None,
        source_value: Any, companion_value: Any,
        source_leaves: dict[str, tuple[Any, str | None]],
        companion_leaves: dict[str, tuple[Any, str | None]],
        source_row: dict[str, Any]) -> str | None:
    """The label for an expected difference at `path` in `category`, or None when it is
    unexpected. `source_origin` is that leaf's origin annotation on the source side (see
    leaf_paths()), checked against a rule's `origins` when it specifies one.
    """
    for pattern, categories, origins, value_condition, label in EXPECTED_DIFFERENCES:
        if categories is not None and category not in categories:
            continue
        if origins is not None and source_origin not in origins:
            continue
        if re.search(pattern, path) and (value_condition is None
                or value_condition(path, source_value, companion_value,
                    source_leaves, companion_leaves, source_row)):
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
        dict[str, ClassStats], list[tuple[str, str, Category, Any, Any]],
        list[tuple[str, str, str]], Counter[str]]:
    """Compares every surveyed class present in either `source` or `companion`.

    Returns per-class ClassStats; the list of
    (class, path, category, source_value, companion_value) leaves not covered by an
    expected-difference rule; and the list of (path, source_font_name,
    companion_font_name) SetFont substitutions found and excused (see
    shape_set_font_paths()) -- excused from the regression count, but still surfaced by
    name rather than silently swallowed as a raw-number mismatch, since a font id
    disagreeing is expected but *which* font it substituted is still worth a look; and a
    count of normalized structural transformations reported outside the leaf diff.
    """
    stats: dict[str, ClassStats] = {}
    unexpected: list[tuple[str, str, Category, Any, Any]] = []
    substitutions: list[tuple[str, str, str]] = []
    # Must run before shape_set_font_paths(source) is used below and before any
    # leaf_paths() call over companion's shape_defs/shape_instruction_lists/shape_data:
    # it mutates companion's own cmpers in place to match source's numbering.
    wrapper_count = realign_shape_cmpers(source, companion)
    transformations: Counter[str] = Counter()
    transformations["Finale-added StartObject wrapper"] = wrapper_count
    source_font_id_paths = shape_set_font_paths(source)
    classes = ((set(source) | set(companion)) - METADATA_KEYS
        - COMPARISON_EXCLUDED_CLASSES)
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
                if (full_path.endswith(".text")
                        and equal_when_companion_omits_nondisplay_font_state(
                            class_name, source_leaves[full_path][0],
                            companion_leaves[full_path][0])):
                    class_stats.same += 1
                    transformations["Finale-omitted non-display text font state"] += 1
                    continue
                if SURVEY_CLASS_POOLS.get(class_name) == "texts" and full_path.endswith(".text"):
                    source_text = source_leaves[full_path][0]
                    companion_text = companion_leaves[full_path][0]
                    source_without_duplicates = normalize_duplicate_enigma_font_states(source_text)
                    companion_without_duplicates = normalize_duplicate_enigma_font_states(companion_text)
                    source_without_time = normalize_enigma_time_inserts(source_without_duplicates)
                    companion_without_time = normalize_enigma_time_inserts(companion_without_duplicates)
                    if (normalize_enigma_font_commands(source_without_time)
                            == normalize_enigma_font_commands(companion_without_time)):
                        class_stats.same += 1
                        if (source_without_duplicates != source_text
                                or companion_without_duplicates != companion_text):
                            transformations["Finale-collapsed duplicate initial font state"] += 1
                        if (source_without_time != source_without_duplicates
                                or companion_without_time != companion_without_duplicates):
                            transformations["Finale-dropped ^time insert"] += 1
                        if source_without_time != companion_without_time:
                            transformations["Enigma font-command spelling"] += 1
                        continue
                category = "differs"
            elif in_source:
                category = "reader_only"
            else:
                category = "companion_only"

            if category == "differs" and full_path in source_font_id_paths:
                source_name = resolve_font_name(source, source_leaves[full_path][0])
                companion_name = resolve_font_name(companion, companion_leaves[full_path][0])
                if source_name is not None and source_name == companion_name:
                    class_stats.same += 1
                    continue
                class_stats.expected_diff += 1
                substitutions.append((full_path, source_name or "?", companion_name or "?"))
                continue

            source_value = source_leaves[full_path][0] if in_source else None
            companion_value = companion_leaves[full_path][0] if in_companion else None
            source_origin = source_leaves[full_path][1] if in_source else None
            label = classify_difference(full_path, category, source_origin,
                source_value, companion_value, source_leaves, companion_leaves, source)
            if label is not None:
                class_stats.expected_diff += 1
                continue

            if category == "differs":
                class_stats.unexpected_diff += 1
            elif category == "reader_only":
                class_stats.reader_only += 1
            else:
                class_stats.companion_only += 1
            unexpected.append((class_name, full_path, category, source_value, companion_value))
    return stats, unexpected, substitutions, transformations


def read_rows(path: Path) -> list[dict]:
    rows = []
    with path.open(encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            rows.append(json.loads(line))
    return rows


def table_widths(headers: list[str], rows: Iterable[list[str]]) -> list[int]:
    widths = [len(h) for h in headers]
    for row in rows:
        for index, cell in enumerate(row):
            widths[index] = max(widths[index], len(cell))
    return widths


def print_table(title: str, headers: list[str], rows: Iterable[list[str]],
        widths: Optional[list[int]] = None) -> None:
    rows = list(rows)
    print(f"\n{title}")
    print("=" * len(title))
    if not rows:
        print("(none)")
        return
    if widths is None:
        widths = table_widths(headers, rows)
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
    all_unexpected_diffs: list[tuple[str, str, str, Any, Any]] = []
    font_substitutions: Counter[tuple[str, str]] = Counter()
    transformations: Counter[str] = Counter()
    for row in companion_rows:
        companion = row["companion"]
        if companion.get("status") != "ok":
            continue
        stats, unexpected, substitutions, row_transformations = compare_row(row, companion)
        transformations.update(row_transformations)
        for class_name, class_stats in stats.items():
            total = totals.setdefault(class_name, ClassStats())
            total.same += class_stats.same
            total.expected_diff += class_stats.expected_diff
            total.unexpected_diff += class_stats.unexpected_diff
            total.reader_only += class_stats.reader_only
            total.companion_only += class_stats.companion_only
        for class_name, path, category, source_value, companion_value in unexpected:
            if category == "differs":
                all_unexpected_diffs.append(
                    (row.get("corpus_id", "?"), class_name, path,
                        source_value, companion_value))
        for _path, source_name, companion_name in substitutions:
            font_substitutions[(source_name, companion_name)] += 1

    print_table("SetFont substitutions (excused from the counts below)",
        ["source font", "companion font", "count"],
        ([source_name, companion_name, str(count)]
            for (source_name, companion_name), count in font_substitutions.most_common()))

    print_table("Normalized companion transformations (excluded from leaf differences)",
        ["transformation", "count"],
        ([name, str(count)] for name, count in transformations.most_common() if count))

    pool_headers = ["class", "same", "expected-diff", "unexpected-diff", "reader-only",
        "companion-only", "total"]
    pool_order = ("options", "others", "details", "texts", "unclassified")
    pool_rows: dict[str, list[list[str]]] = {}
    for pool in pool_order:
        pool_rows[pool] = [
            [name, str(stats.same), str(stats.expected_diff), str(stats.unexpected_diff),
                str(stats.reader_only), str(stats.companion_only), str(stats.total())]
            for name, stats in sorted(totals.items())
            if SURVEY_CLASS_POOLS.get(name, "unclassified") == pool]
    shared_pool_widths = table_widths(pool_headers,
        (row for rows_for_pool in pool_rows.values() for row in rows_for_pool))

    printed_pool = False
    for pool in pool_order:
        if not pool_rows[pool]:
            continue
        if printed_pool:
            print("\n" + "-" * 80)
        print_table(f"{pool} pool companion comparison (leaves)",
            pool_headers, pool_rows[pool], shared_pool_widths)
        printed_pool = True

    if all_unexpected_diffs:
        shown = all_unexpected_diffs[:max_unexpected]
        print_table(
            f"Unexpected differences (first {len(shown)} of {len(all_unexpected_diffs)})",
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
