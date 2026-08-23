#!/usr/bin/env python3
"""Summarize recovery_coverage_probe's JSON Lines output as plain text tables.

No LLM in the loop: every table here is a deterministic reduction over the probe's own
output, meant to replace asking an assistant to eyeball a jq query and describe what it
found. Two things are summarized:

  - what the reader recovered: status/epoch counts and failure reasons, all counted
    across every row in the file.
  - how it compares to Finale's own companion, for rows the probe ran with a
    `#companion:`-declared corpus (see recovery_coverage_probe.cpp): same /
    expected-difference / unexpected-difference / reader-only / companion-only counts per
    surveyed class, plus a separate chunk-based Enigma-text comparison.

The companion comparison walks every leaf under each surveyor's output (skipping
`origin_*` annotations, which exist only on the source side and would differ on every row
by construction) and classifies each non-match through EXPECTED_DIFFERENCES below. That
table starts small and is meant to grow: an "unexpected" count is not necessarily a
regression, it is the next entry to characterize and, if it is a real known difference, add
a rule for. This script is the maintained home for those exceptions.
"""

from __future__ import annotations

import argparse
import json
import re
import sys
import time
from collections import Counter
from dataclasses import dataclass, replace
from functools import cache
from pathlib import Path
from typing import Any, Callable, Iterable, Iterator, Optional

# Row keys that describe the row/import itself rather than surveyed content. Excluded from
# both the per-class breakdown and the recursive companion diff.
METADATA_KEYS = {
    "corpus_id", "status", "epoch", "saving_product", "source_version", "header",
    "warning_count", "diagnostics", "duration_ms", "timings", "companion", "finder_type", "error",
}

# Surveyors retained in the JSONL for recovery exploration but not mature enough for
# companion-quality claims yet. Excluding them here avoids presenting proof-of-concept
# fields as reader regressions while preserving the snapshot for later analysis.
COMPARISON_EXCLUDED_CLASSES = {
    "layer_atts",
    "relationships",
    "spacing_options",
    "text_metadata",
}

TEXT_BLOCKS_SURVEYOR = "text_blocks"

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
    TEXT_BLOCKS_SURVEYOR: "others",
    "text_options": "options",
}

# Enigma-text comparison belongs exclusively to the texts pool. TextBlocks expose their text
# referent as text_id and are compared independently by their own cmpers.
ENIGMA_TEXT_SURVEYORS = frozenset(
    class_name for class_name, pool in SURVEY_CLASS_POOLS.items() if pool == "texts"
)

# Legacy Finale music-font names documented in Finale's public font table:
# https://usermanuals.finalemusic.com/FinaleWin/Content/Finale/ht-fonts.htm
# This is an analysis aid, not a reader rule. Finale's MacSymbolFonts.txt is
# user-editable, so this embedded stock set intentionally does not claim to be
# the complete installation-local list.
FINALE_STOCK_SYMBOL_FONT_NAMES = {
    "maestro", "maestropercussion", "maestrowide",
    "broadwaycopyist", "broadwaycopyistperc", "broadwaycopyisttext",
    "engraverfontextras", "engravertime",
    "jazz", "jazzcord", "jazzperc", "jazztext",
    "finalepercussion", "finalemallets", "finalealphanotes",
    "petrucci", "seville", "tamburo",
}

# Cross-locale font spellings observed in Finale upgrades. The key and value use the
# normalized-name form stored in the coverage snapshot; add an alias only when both
# spellings have been independently observed for the same font.
FONT_NAME_ALIASES = {
    "ヒラギノ明朝prow3": "hiraginominchoprow3",
    "ヒラギノ丸ゴprow4": "hiraginomarugothicpro",
    "newcenturyschlbk": "newcenturyschoolbook",
}

UNEXPECTED_VALUE_DISPLAY_WIDTH = 60
PROGRESS_INTERVAL_SECONDS = 1.9


@cache
def is_noncontent_key(key: str) -> bool:
    """True when a key has the same non-content meaning wherever it appears.

    Field origins and legacy byte offsets are recovery diagnostics. Indices and cmpers
    identify leaves; counting them again as values only multiplies a one-sided object
    difference. Aggregate instruction/data counts duplicate the collections themselves.
    All remain useful in the JSONL but none participates in companion comparison. Dangling
    reference flags deliberately remain comparable: they can expose a lost referent much
    more clearly than a large one-sided pool count.
    """
    return (key in {"origin", "index", "cmper", "_report_match_key",
            "instruction_count", "value_count",
            "external_graphic_count", "undocumented_instruction_count"}
        or key.startswith("origin_") or key.endswith("_origin")
        or key.endswith("_block_offset") or key.endswith("_decoded_field_offset"))


# Fully qualified fields or subtrees that are useful recovery diagnostics but are not
# companion-comparable document content. Unlike is_noncontent_key(), these names have that
# meaning only at their stated path. A path excludes itself and everything nested below it.
COMPARISON_EXCLUDED_PATHS = {
    # Tuple provenance comes from ImportReport fields, which an independently parsed
    # companion does not have, so its survey currently emits no corresponding tuples.
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
    # ShapeDefs are paired by the contents of their instruction and data referents.
    # Finale may renumber or consolidate identical referents independently, so comparing
    # their document-local keys after that semantic match has no remaining meaning.
    re.compile(r"^shape_defs\[[^\]]+\]\.(instruction_list|data_list)$"),
]

# A legacy source names a font in its Enigma string, while Finale may rewrite that same
# selection as a document-local font id or category. The chunk parser resolves those
# spellings to one identity and accumulates the commands that govern text-run formatting.
ENIGMA_TIME_INSERT = re.compile(r"\^time\([^)]*\)")
ENIGMA_FONT_STATE_COMMAND_START = re.compile(
    r"\^(?P<kind>font|fontid|Font|fontMus|fontTxt|fontNum|size|nfx|efx|"
    r"tracking|baseline|superscript)"
    r"\(")
ENIGMA_FONT_KINDS = frozenset({"font", "fontid", "Font", "fontMus", "fontTxt", "fontNum"})

BLOCK_TEXT_PATH = re.compile(r"^block_texts\[number=(\d+)\]\.text$")
TEXT_PATH_TO_TARGET = {
    "block_texts": "blockText",
    "bookmark_texts": "bookmarkText",
    "expression_texts": "expression",
    "file_info_texts": "fileInfo",
    "lyrics_choruses": "lyricsChorus",
    "lyrics_sections": "lyricsSection",
    "lyrics_verses": "lyricsVerse",
    "smart_shape_texts": "smartShapeText",
}
TEXT_PATH = re.compile(r"^(?P<class>[^[]+)\[number=(?P<number>\d+)\]\.text$")
LEGACY_TEXT_WHITESPACE_CONTROLS = str.maketrans({"\x01": None, "\x06": None})


@cache
def normalize_font_name(value: str | None) -> str | None:
    """Returns the report's cross-locale identity for a normalized font name."""
    if not isinstance(value, str):
        return value
    folded = value.casefold()
    return FONT_NAME_ALIASES.get(folded, folded)


def normalize_legacy_text_whitespace(value: Any) -> Any:
    """Removes legacy controls that Finale treats as whitespace when upgrading text."""
    return (value.translate(LEGACY_TEXT_WHITESPACE_CONTROLS)
        if isinstance(value, str) else value)


def normalize_enigma_time_inserts(value: Any) -> Any:
    """Removes the time insert that Finale drops while upgrading legacy text."""
    return ENIGMA_TIME_INSERT.sub("", value) if isinstance(value, str) else value


def encode_legacy_text(value: str, encoding: str) -> bytes:
    """Encodes text while preserving undefined Windows-1252 bytes recovered as controls."""
    if encoding not in {"cp1252", "windows-1252"}:
        return value.encode(encoding)
    result = bytearray()
    for character in value:
        try:
            result.extend(character.encode("windows-1252"))
        except UnicodeEncodeError:
            codepoint = ord(character)
            if codepoint not in {0x81, 0x8d, 0x8f, 0x90, 0x9d}:
                raise
            result.append(codepoint)
    return bytes(result)



def relationship_text_ids(row: dict, role: str) -> tuple[int, set[int]]:
    """Returns the role's document coverage and its distinct text-pool references."""
    relationship = (row.get("relationships") or {}).get(role) or {}
    return (relationship.get("total_parts", 0), set(relationship.get("text_ids") or []))


def is_part_name_text(path: str, source_row: dict, companion_row: dict) -> bool:
    """Returns whether the available document relationships identify a part-name text."""
    match = BLOCK_TEXT_PATH.fullmatch(path)
    if match is None:
        return False
    text_id = int(match.group(1))
    source_parts, source_ids = relationship_text_ids(source_row, "part_names")
    companion_parts, companion_ids = relationship_text_ids(companion_row, "part_names")
    if source_parts and companion_parts:
        return text_id in source_ids and text_id in companion_ids
    if source_parts:
        return text_id in source_ids
    return text_id in companion_ids


def equal_part_name_text(path: str, source_value: Any, companion_value: Any,
        source_row: dict, companion_row: dict, source_chunks: list[EnigmaChunk] | None,
        companion_chunks: list[EnigmaChunk] | None) -> bool:
    """Ignores initial formatting only for a structurally identified part-name text.

    The source relationship wins once the reader recovers PartDefinition and TextBlock.
    Until then, the independently parsed companion supplies the role. When both sides
    provide the relationship they must agree, so a relationship mismatch cannot silently
    excuse a text difference.
    """
    if (not isinstance(source_value, str) or not isinstance(companion_value, str)
            or not is_part_name_text(path, source_row, companion_row)):
        return False
    return (source_chunks is not None and companion_chunks is not None
        and "".join(chunk.text for chunk in source_chunks)
            == "".join(chunk.text for chunk in companion_chunks))


def is_defaulted_part_name_text(path: str, source_row: dict, companion_row: dict,
        source_chunks: list[EnigmaChunk] | None,
        companion_chunks: list[EnigmaChunk] | None) -> bool:
    """Recognizes Finale supplying its default score name for an empty part-name text."""
    if (source_chunks is None or companion_chunks is None
            or not is_part_name_text(path, source_row, companion_row)):
        return False
    return ("".join(chunk.text for chunk in source_chunks) == ""
        and "".join(chunk.text for chunk in companion_chunks) == "Score")


def synthesized_text_state(source_row: dict, class_name: str, path: str) -> dict | None:
    """Returns source text provenance for one surveyed text path, if available."""
    match = TEXT_PATH.fullmatch(path)
    node_name = TEXT_PATH_TO_TARGET.get(class_name)
    if not match or node_name is None:
        return None
    target = (f"texts.{node_name}[{match.group('number')}].text")
    return (source_row.get("text_metadata") or {}).get(target)


def synthesized_initial_state_fields(metadata: dict) -> frozenset[str]:
    """Returns the initial chunk-state fields supplied by the reader."""
    fields = set()
    if metadata.get("font_synthesized"):
        fields.add("font")
    if metadata.get("size_synthesized"):
        fields.add("size")
    if metadata.get("effects_synthesized"):
        fields.add("effects")
    return frozenset(fields)


def equal_after_synthesized_initial_state(path: str, class_name: str,
        source_value: Any, companion_value: Any, source_row: dict,
        companion_row: dict) -> bool:
    """Recognizes differences caused solely by reader-completed initial text state."""
    metadata = synthesized_text_state(source_row, class_name, path)
    if (metadata is None or not isinstance(source_value, str)
            or not isinstance(companion_value, str)):
        return False
    ignored = synthesized_initial_state_fields(metadata)
    if not ignored:
        return False
    source_chunks = parse_enigma_chunks(
        normalize_enigma_time_inserts(source_value), source_row, ignored)
    companion_chunks = parse_enigma_chunks(
        normalize_enigma_time_inserts(companion_value), companion_row, ignored)
    return enigma_chunks_equal(source_chunks, companion_chunks)


FontReference = tuple[str, Any]  # ("id", cmper) or ("name", spelling)


def font_definition(row: dict, reference: FontReference) -> dict | None:
    """Resolves one ID or spelling through this row's own font-definition table."""
    definitions = (row.get("font_definitions") or {}).get("definitions") or []
    kind, value = reference
    if kind == "id":
        return next((item for item in definitions if item.get("cmper") == value), None)
    identity = normalize_font_name(value)
    by_name = next((item for item in definitions
        if normalize_font_name(item.get("name")) == identity
        or normalize_font_name(item.get("normalized_name")) == identity), None)
    if by_name:
        return by_name
    # Finale may spell a document-local comparator as `font(FontN,...)` rather than
    # `fontid(N)`. An actual definition carrying that literal face name wins above.
    placeholder = re.fullmatch(r"font(\d+)", identity)
    return (next((item for item in definitions
        if item.get("cmper") == int(placeholder.group(1))), None) if placeholder else None)


def font_identity(row: dict, reference: FontReference) -> str | None:
    """Returns musxdom's normalized identity for one font reference."""
    definition = font_definition(row, reference)
    if not definition:
        return None
    # Comparator 0 is the default music font as a role, even when a save gives its face a
    # placeholder spelling such as Font0. This is the same identity rule used to align the
    # font-definition arrays in _font_definition_key().
    return ("<default-music-font>" if definition.get("cmper") == 0 else
        normalize_font_name(definition.get("normalized_name")))


def fonts_equal(source_row: dict, source_reference: FontReference,
        companion_row: dict, companion_reference: FontReference) -> bool:
    """The report's sole decision point for semantic font equality."""
    source_identity = font_identity(source_row, source_reference)
    return (source_identity is not None
        and source_identity == font_identity(companion_row, companion_reference))


def enigma_font_reference(kind: str, argument: str) -> FontReference | None:
    """Parses one font-state command into a document-local font reference."""
    name_or_id = argument.split(",", 1)[0]
    if kind == "fontid":
        try:
            return ("id", int(name_or_id))
        except ValueError:
            return None
    return ("name", name_or_id)


@dataclass(frozen=True)
class EnigmaFontState:
    """The accumulated font and run-formatting state governing one Enigma text chunk."""
    font: Any = None
    definition_cmper: int | None = None
    charset: tuple[Any, Any] | None = None
    explicit_charset: str | None = None
    size: int | None = None
    effects: int | None = None
    tracking: int = 0
    baseline: int = 0
    superscript: int = 0


@dataclass(frozen=True)
class EnigmaChunk:
    text: str
    state: EnigmaFontState


def enigma_state_signature(state: EnigmaFontState) -> tuple[Any, ...]:
    """Returns the command-controlled values that determine chunk equivalence."""
    return (state.font, state.size, state.effects,
        state.tracking, state.baseline, state.superscript)


def enigma_font_state_command(
        value: str, at: int) -> tuple[str, str, int, str] | None:
    """Returns kind, argument, end, and spelling for one balanced state command."""
    start = ENIGMA_FONT_STATE_COMMAND_START.match(value, at)
    if start is None:
        return None
    depth = 1
    cursor = start.end()
    while cursor < len(value):
        if value[cursor] == "(":
            depth += 1
        elif value[cursor] == ")":
            depth -= 1
            if depth == 0:
                end = cursor + 1
                return (start.group("kind"), value[start.end():cursor], end,
                    value[at:end])
        cursor += 1
    return None


def parse_enigma_chunks(value: Any, row: dict,
        ignored_initial_state: frozenset[str] = frozenset()) -> list[EnigmaChunk] | None:
    """Parses text into chunks governed by accumulated font and run-formatting state.

    Commands outside that state remain literal text. Consecutive state commands accumulate
    before the next chunk, and a command that restates its current value is discarded. This
    follows the chunking model of musxdom's Enigma parser without callbacks or insert
    interpretation.
    """
    if not isinstance(value, str):
        return None
    chunks: list[EnigmaChunk] = []
    state = EnigmaFontState()
    text_parts: list[str] = []
    saw_state = False
    initial_state_commands = True
    at = 0

    def flush() -> None:
        if text_parts:
            chunks.append(EnigmaChunk("".join(text_parts), state))
            text_parts.clear()

    while at < len(value):
        caret = value.find("^", at)
        if caret < 0:
            if value[at:]:
                text_parts.append(value[at:])
                initial_state_commands = False
            break
        literal = value[at:caret]
        if literal:
            text_parts.append(literal)
            initial_state_commands = False
        if caret + 1 < len(value) and value[caret + 1] == "^":
            text_parts.append("^^")
            initial_state_commands = False
            at = caret + 2
            continue
        command = enigma_font_state_command(value, caret)
        if command is None:
            text_parts.append("^")
            initial_state_commands = False
            at = caret + 1
            continue

        kind, argument, command_end, command_text = command
        field = ("font" if kind in ENIGMA_FONT_KINDS else
            "effects" if kind in {"nfx", "efx"} else kind)
        if initial_state_commands and field in ignored_initial_state:
            at = command_end
            continue
        next_state = state
        if kind in ENIGMA_FONT_KINDS:
            reference = enigma_font_reference(kind, argument)
            definition = font_definition(row, reference) if reference else None
            identity = (font_identity(row, reference) if reference else None)
            if identity is None:
                identity = ("unresolved", kind, normalize_font_name(argument.split(",", 1)[0]))
            explicit_charset = argument.rsplit(",", 1)[-1] if "," in argument else None
            next_state = replace(state, font=identity,
                definition_cmper=definition.get("cmper") if definition else None,
                charset=((definition.get("charset_bank"), definition.get("charset_value"))
                    if definition else None), explicit_charset=explicit_charset)
        else:
            try:
                number = int(argument)
            except ValueError:
                text_parts.append(command_text)
                at = command_end
                continue
            next_state = replace(state, **{field: number})

        if enigma_state_signature(next_state) != enigma_state_signature(state):
            flush()
        state = next_state
        saw_state = True
        at = command_end

    flush()
    if saw_state and not chunks:
        chunks.append(EnigmaChunk("", state))
    elif (saw_state and not text_parts and chunks
            and enigma_state_signature(chunks[-1].state) != enigma_state_signature(state)):
        chunks.append(EnigmaChunk("", state))
    return chunks


def enigma_chunks_equal(source_chunks: list[EnigmaChunk] | None,
        companion_chunks: list[EnigmaChunk] | None) -> bool:
    if source_chunks is None or companion_chunks is None or len(source_chunks) != len(companion_chunks):
        return False
    return all(source.text == companion.text
        and enigma_state_signature(source.state) == enigma_state_signature(companion.state)
        for source, companion in zip(source_chunks, companion_chunks))


def enigma_text_has_missing_run(source_chunks: list[EnigmaChunk] | None,
        companion_chunks: list[EnigmaChunk] | None) -> bool:
    """Returns whether either chunk stream is the other with complete text runs removed."""
    if source_chunks is None or companion_chunks is None:
        return False

    def is_proper_subsequence(shorter: list[EnigmaChunk], longer: list[EnigmaChunk]) -> bool:
        if len(shorter) >= len(longer):
            return False
        shorter_at = 0
        unmatched_nonempty = False
        for chunk in longer:
            if (shorter_at < len(shorter)
                    and enigma_chunks_equal([shorter[shorter_at]], [chunk])):
                shorter_at += 1
            elif chunk.text:
                unmatched_nonempty = True
        return shorter_at == len(shorter) and unmatched_nonempty

    return (is_proper_subsequence(source_chunks, companion_chunks)
        or is_proper_subsequence(companion_chunks, source_chunks))


def enigma_chunks_signature(value: Any, row: dict) -> tuple | None:
    """Returns the hashable semantic identity used by Enigma chunk comparison."""
    chunks = parse_enigma_chunks(value, row)
    if chunks is None:
        return None
    return tuple((chunk.text, enigma_state_signature(chunk.state)) for chunk in chunks)


def enigma_chunk_differences(source_chunks: list[EnigmaChunk] | None,
        companion_chunks: list[EnigmaChunk] | None) -> set[str]:
    """Returns independent font-tuple and text differences between two chunk streams."""
    if source_chunks is None or companion_chunks is None:
        return {"text"}
    differences = set()
    source_text = "".join(chunk.text for chunk in source_chunks)
    companion_text = "".join(chunk.text for chunk in companion_chunks)
    if source_text != companion_text:
        differences.add("text")

    state_pairs: list[tuple[EnigmaFontState, EnigmaFontState]] = []
    if source_text == companion_text:
        def spans(chunks: list[EnigmaChunk]) -> list[tuple[int, int, EnigmaFontState]]:
            result = []
            offset = 0
            for chunk in chunks:
                result.append((offset, offset + len(chunk.text), chunk.state))
                offset += len(chunk.text)
            return result

        source_spans = spans(source_chunks)
        companion_spans = spans(companion_chunks)
        boundaries = sorted({0, len(source_text),
            *(end for _start, end, _state in source_spans),
            *(end for _start, end, _state in companion_spans)})

        def state_at(items: list[tuple[int, int, EnigmaFontState]], position: int) -> EnigmaFontState:
            for start, end, state in items:
                if start <= position < end:
                    return state
            return items[-1][2] if items else EnigmaFontState()

        positions = boundaries[:-1] if len(boundaries) > 1 else [0]
        state_pairs = [(state_at(source_spans, position),
            state_at(companion_spans, position)) for position in positions]
        state_pairs.append((state_at(source_spans, len(source_text)),
            state_at(companion_spans, len(companion_text))))
    else:
        state_pairs = [(source.state, companion.state)
            for source, companion in zip(source_chunks, companion_chunks)]

    for source_state, companion_state in state_pairs:
        if source_state.font != companion_state.font:
            differences.add("font")
        if source_state.size != companion_state.size:
            differences.add("size")
        if source_state.effects != companion_state.effects:
            differences.add("effects")
        if (source_state.tracking != companion_state.tracking
                or source_state.baseline != companion_state.baseline
                or source_state.superscript != companion_state.superscript):
            differences.add("other")
    return differences


def wrong_platform_encoding_by_font_run(
        chunks: list[EnigmaChunk]) -> tuple[list[EnigmaChunk], frozenset[str]] | None:
    """Applies Finale's observed wrong legacy decoding inside each eligible font chunk."""
    if not chunks:
        return None
    converted: list[EnigmaChunk] = []
    conversions: set[str] = set()
    for chunk in chunks:
        run = chunk.text
        charset = chunk.state.charset
        encodings = ({
            (0, 0): ("mac_roman", "windows-1252", "mac-roman-to-windows-1252"),
            (0, 29): ("mac_latin2", "mac_roman", "mac-ce-to-mac-roman"),
            (1, 0): ("windows-1252", "mac_roman", "windows-1252-to-mac-roman"),
            (1, 1): ("windows-1252", "mac_roman", "windows-default-to-mac-roman"),
            (1, 238): ("cp1250", "mac_roman", "windows-1250-to-mac-roman"),
        }).get(charset)
        if encodings:
            try:
                misdecoded = encode_legacy_text(run, encodings[0]).decode(encodings[1])
            except UnicodeError:
                return None
            converted.append(EnigmaChunk(misdecoded, chunk.state))
            if misdecoded != run:
                conversions.add(encodings[2])
        else:
            converted.append(chunk)
    return (converted, frozenset(conversions)) if conversions else None


def finale_wrong_platform_encoding(source_chunks: list[EnigmaChunk] | None,
        companion_chunks: list[EnigmaChunk] | None) -> str | None:
    """Labels an exact font-run-aware legacy-encoding misdecoding by Finale.

    Both complete font-command sequences must resolve to the same named fonts. Each source
    run with an observed wrong-decoding pair is round-tripped through its stated encoding and
    decoded through Finale's apparent target encoding; other runs are retained. The complete
    result must equal the normalized companion, so partial substitutions never qualify.
    """
    if (source_chunks is None or companion_chunks is None
            or len(source_chunks) != len(companion_chunks)):
        return None
    if not all(enigma_state_signature(source.state)
            == enigma_state_signature(companion.state)
            for source, companion in zip(source_chunks, companion_chunks)):
        return None
    converted = wrong_platform_encoding_by_font_run(source_chunks)
    if converted is None or not enigma_chunks_equal(converted[0], companion_chunks):
        return None
    labels = {
        frozenset({"mac-roman-to-windows-1252"}):
            "Finale decoded Mac Roman text as Windows-1252",
        frozenset({"windows-1252-to-mac-roman"}):
            "Finale decoded Windows-1252 text as Mac Roman",
        frozenset({"windows-default-to-mac-roman"}):
            "Finale resolved ambiguous Windows DEFAULT_CHARSET text as Mac Roman",
        frozenset({"mac-ce-to-mac-roman"}):
            "Finale decoded Mac Central European text as Mac Roman",
        frozenset({"windows-1250-to-mac-roman"}):
            "Finale decoded Windows-1250 text as Mac Roman",
        frozenset({"mac-ce-to-mac-roman", "windows-1250-to-mac-roman"}):
            "Finale decoded Central European text as Mac Roman",
        frozenset({"mac-roman-to-windows-1252", "windows-1252-to-mac-roman"}):
            "Finale swapped Roman platform encodings by font run",
    }
    return labels.get(converted[1])


def finale_symbol_font_encoding_glitch(source_chunks: list[EnigmaChunk] | None,
        companion_chunks: list[EnigmaChunk] | None) -> bool:
    """Recognizes Finale decoding symbol-font glyph bytes through a text charset."""
    if (not source_chunks or not companion_chunks
            or len(source_chunks) != len(companion_chunks)):
        return False
    affected = False
    for source_chunk, companion_chunk in zip(source_chunks, companion_chunks):
        source_state = source_chunk.state
        companion_state = companion_chunk.state
        if enigma_state_signature(source_state) != enigma_state_signature(companion_state):
            return False
        source_charset = source_state.charset[1] if source_state.charset else None
        companion_charset = companion_state.charset[1] if companion_state.charset else None
        is_affected = ((source_state.font in FINALE_STOCK_SYMBOL_FONT_NAMES
                and source_charset != 4095 and companion_charset == 4095)
            or (source_state.explicit_charset is None
                and companion_state.explicit_charset in {"8191", "8192"}))
        if source_chunk.text != companion_chunk.text:
            byte_preserved = False
            if source_state.charset in {(0, 4095), (1, 2)}:
                try:
                    byte_preserved = bytes(ord(character) for character in source_chunk.text).decode(
                        "mac_roman") == companion_chunk.text
                except (UnicodeDecodeError, ValueError):
                    pass
            if not is_affected and not byte_preserved:
                return False
            affected = True
    return affected


def finale_utf16le_byte_pair_glitch(source_chunks: list[EnigmaChunk] | None,
        companion_chunks: list[EnigmaChunk] | None) -> bool:
    """Recognizes Finale reading legacy single-byte runs as UTF-16LE code units.

    Commands and other unaffected ASCII may surround the damaged run, so only characters
    above U+00FF are split back into low/high bytes. Each run is decoded through the source
    chunk's own Roman charset, and every reconstructed chunk must then match exactly.
    """
    if (not source_chunks or not companion_chunks
            or len(source_chunks) != len(companion_chunks)):
        return False
    affected = False
    for source_chunk, companion_chunk in zip(source_chunks, companion_chunks):
        encoding = {
            (0, 0): "mac_roman",
            (1, 0): "windows-1252",
        }.get(source_chunk.state.charset)
        if encoding is None:
            continue
        rebuilt: list[str] = []
        paired = bytearray()
        paired_characters = 0

        def flush_paired() -> bool:
            if not paired:
                return True
            try:
                rebuilt.append(paired.decode(encoding))
            except UnicodeError:
                return False
            paired.clear()
            return True

        for character in companion_chunk.text:
            codepoint = ord(character)
            if 0xff < codepoint <= 0xffff:
                paired.extend((codepoint & 0xff, codepoint >> 8))
                paired_characters += 1
            else:
                if not flush_paired():
                    return False
                rebuilt.append(character)
        if not flush_paired():
            continue
        rebuilt_text = "".join(rebuilt).replace("\r\n", "\n").replace("\r", "\n")
        if paired_characters >= 2 and source_chunk.text == rebuilt_text:
            return True

        # A binary Enigma command inside the misread run can terminate Finale's wide-string
        # conversion and discard that command plus adjacent literal bytes. Accept that lossy
        # form only when a substantial reconstructed plain-text sequence survives in order;
        # arbitrary substitutions cannot pass this test.
        strip_commands = lambda value: re.sub(r"\^[A-Za-z]+\([^)]*\)", "", value)
        source_plain = strip_commands(source_chunk.text)
        rebuilt_plain = strip_commands(rebuilt_text)
        meaningful = "".join(character for character in rebuilt_plain
            if not character.isspace())
        source_at = 0
        for character in rebuilt_plain:
            source_at = source_plain.find(character, source_at)
            if source_at < 0:
                break
            source_at += 1
        else:
            if paired_characters >= 2 and len(meaningful) >= 8:
                return True
    return False


def equal_empty_enigma_text(source_value: Any, companion_value: Any,
        source_chunks: list[EnigmaChunk] | None,
        companion_chunks: list[EnigmaChunk] | None) -> bool:
    """Treats font-state-only text as semantically empty."""
    return (source_chunks is not None and companion_chunks is not None
        and source_value != companion_value
        and not any(chunk.text for chunk in source_chunks)
        and not any(chunk.text for chunk in companion_chunks))


def equal_unformatted_metadata_text(class_name: str, source_value: Any,
        companion_value: Any, source_chunks: list[EnigmaChunk] | None,
        companion_chunks: list[EnigmaChunk] | None) -> bool:
    """Treats a metadata text's plain spelling as equal to a font-prefixed spelling."""
    if class_name not in {"file_info_texts", "bookmark_texts"}:
        return False
    if not isinstance(source_value, str) or not isinstance(companion_value, str):
        return False
    if source_chunks is None or companion_chunks is None:
        return False
    source_plain = "".join(chunk.text for chunk in source_chunks)
    companion_plain = "".join(chunk.text for chunk in companion_chunks)
    source_has_state = any(enigma_state_signature(chunk.state) != (None, None, None)
        for chunk in source_chunks)
    companion_has_state = any(enigma_state_signature(chunk.state) != (None, None, None)
        for chunk in companion_chunks)
    return source_has_state != companion_has_state and source_plain == companion_plain


def equal_symbol_font_platform_spelling(path: str,
        source_leaves: dict[str, tuple[Any, Optional[str]]],
        companion_leaves: dict[str, tuple[Any, Optional[str]]]) -> bool:
    """Recognizes equivalent Windows-symbol and Mac-symbol font descriptors.

    Every leaf is ambiguous alone, so the same normalized font definition must carry the
    complete `(bank, charset, pitch)` descriptor `(1, 2, 2)` and `(0, 4095, 0)`, in either
    direction.
    """
    if not path.endswith((".charset_bank", ".charset_value", ".pitch")):
        return False
    prefix = path.rsplit(".", 1)[0]
    def descriptor(leaves: dict[str, tuple[Any, Optional[str]]]) -> tuple[Any, Any, Any]:
        return (leaves.get(prefix + ".charset_bank", (None, None))[0],
            leaves.get(prefix + ".charset_value", (None, None))[0],
            leaves.get(prefix + ".pitch", (None, None))[0])
    return {descriptor(source_leaves), descriptor(companion_leaves)} == {
        (1, 2, 2), (0, 4095, 0)}


def is_comparison_excluded_path(path: str) -> bool:
    # leaf_paths() stops descending as soon as it reaches an excluded subtree, so descendants
    # never arrive here and exact membership is sufficient for those paths. The patterned
    # exclusions belong to only two classes; avoid running both regexes over every leaf in
    # every other class.
    if path in COMPARISON_EXCLUDED_PATHS:
        return True
    if not path.startswith(("font_definitions.", "shape_defs[")):
        return False
    return any(pattern.search(path) for pattern in COMPARISON_EXCLUDED_PATH_PATTERNS)


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
    """Recognizes Finale converting a font descriptor to a platform symbol charset.

    `(bank 0, value 4095)` is Mac symbol and `(bank 1, value 2)` is Windows symbol.
    The target descriptor therefore states the result directly; no stock-font-name list or
    saving-version heuristic is needed. This rule classifies the charset-value leaf. A bank
    change between the two platform spellings is handled by platform-charset-numbering.
    """
    if source_value == companion_value:
        return False
    bank_path = path.removesuffix(".charset_value") + ".charset_bank"
    companion_bank = companion_leaves.get(bank_path, (None, None))[0]
    return (companion_bank, companion_value) in {(0, 4095), (1, 2)}


def is_finale_platform_charset_normalization(path: str, source_value: Any,
        companion_value: Any, source_leaves: dict[str, tuple[Any, str | None]],
        companion_leaves: dict[str, tuple[Any, str | None]], _source_row: dict[str, Any]) -> bool:
    """Recognizes Finale's platform charset numbering conversion in either direction."""
    if path.endswith(".charset_bank"):
        return ((source_value == 0 and companion_value == 1)
            or (source_value == 1 and companion_value == 0))
    if path.endswith(".charset_value"):
        bank_path = path.removesuffix(".charset_value") + ".charset_bank"
        return (source_value == 1 and companion_value == 2
            and source_leaves.get(bank_path, (None, None))[0] == 1
            and companion_leaves.get(bank_path, (None, None))[0] == 1)
    return False


def is_finale_symbol_glyph_reinterpretation(path: str, source_value: Any,
        companion_value: Any, _source_leaves: dict[str, tuple[Any, str | None]],
        _companion_leaves: dict[str, tuple[Any, str | None]], source_row: dict[str, Any]) -> bool:
    """Recognizes Finale reinterpreting a decoded text character as a symbol-font byte.

    A legacy font definition can claim an ordinary platform encoding while Finale's
    installation-local symbol-font configuration overrides it during upgrade. This is accepted
    only when both connections name the same normalized font, the companion definition states a
    symbol charset, and encoding the source code point through its stated legacy charset recovers
    the companion's exact byte. Other character changes remain unexpected.
    """
    if (not isinstance(source_value, int)
            or not isinstance(companion_value, int) or not 0 <= companion_value <= 0xff):
        return False
    companion_row = source_row.get("companion") or {}

    source_font_id = companion_font_id = None
    stem_match = re.fullmatch(r"stem_options\.stem_connections\[(\d+)\]\.symbol", path)
    line_match = re.fullmatch(r"ss_line_styles\[cmper=(\d+)\]\.line_char", path)
    if stem_match:
        index = int(stem_match.group(1))
        source_items = (source_row.get("stem_options") or {}).get("stem_connections") or []
        companion_items = (companion_row.get("stem_options") or {}).get("stem_connections") or []
        if index >= len(source_items) or index >= len(companion_items):
            return False
        source_font_id = source_items[index].get("font_id")
        companion_font_id = companion_items[index].get("font_id")
    elif line_match:
        cmper = int(line_match.group(1))
        source_item = next((item for item in source_row.get("ss_line_styles") or []
            if item.get("cmper") == cmper), None)
        companion_item = next((item for item in companion_row.get("ss_line_styles") or []
            if item.get("cmper") == cmper), None)
        if source_item is None or companion_item is None:
            return False
        source_font_id = source_item.get("char_font_id")
        companion_font_id = companion_item.get("char_font_id")
    else:
        return False

    source_definition = font_definition(source_row, ("id", source_font_id))
    companion_definition = font_definition(companion_row, ("id", companion_font_id))
    if source_definition is None or companion_definition is None:
        return False
    source_name = normalize_font_name(source_definition.get("name"))
    companion_name = normalize_font_name(companion_definition.get("name"))
    if not source_name or source_name != companion_name:
        return False
    source_descriptor = (source_definition.get("charset_bank"),
        source_definition.get("charset_value"))
    companion_descriptor = (companion_definition.get("charset_bank"),
        companion_definition.get("charset_value"))
    if companion_descriptor not in {(0, 4095), (1, 2)}:
        return False
    source_codec = {(0, 0): "mac_roman", (1, 0): "cp1252"}.get(source_descriptor)
    if source_codec is None:
        return False
    try:
        return encode_legacy_text(chr(source_value), source_codec) == bytes([companion_value])
    except (UnicodeEncodeError, ValueError):
        return False


def is_empty_companion_partname_template(_path: str, source_value: Any,
        companion_value: Any, _source_leaves: dict[str, tuple[Any, str | None]],
        _companion_leaves: dict[str, tuple[Any, str | None]], _source_row: dict[str, Any]) -> bool:
    """Recognizes an unresolved block-text-only part-name template."""
    if companion_value != "" or not isinstance(source_value, str):
        return False
    return re.fullmatch(
        r"(?:\^(?:font|fontid|Font|fontMus|fontTxt|fontNum|size|nfx)\([^)]*\))*"
        r"\^partname\(\)", source_value) is not None


def is_whitespace_text_difference(path: str, source_value: Any,
        companion_value: Any, _source_leaves: dict[str, tuple[Any, str | None]],
        _companion_leaves: dict[str, tuple[Any, str | None]], source_row: dict[str, Any],
        companion_row: dict[str, Any], source_chunks: list[EnigmaChunk] | None,
        companion_chunks: list[EnigmaChunk] | None) -> bool:
    """Recognizes Finale's formatting-only whitespace changes."""
    if not isinstance(source_value, str) or not isinstance(companion_value, str):
        return False
    if source_chunks is None or companion_chunks is None:
        return False

    def nonwhitespace_runs(chunks: list[EnigmaChunk]) -> list[tuple[str, tuple[Any, ...]]]:
        runs = []
        for chunk in chunks:
            for text in re.split(r"(\s+)", chunk.text):
                if text and not text.isspace():
                    runs.append((text, enigma_state_signature(chunk.state)))
        return runs

    return (source_value != companion_value
        and nonwhitespace_runs(source_chunks) == nonwhitespace_runs(companion_chunks)
        and (any(character.isspace() for character in source_value)
            or any(character.isspace() for character in companion_value)))


@dataclass(frozen=True)
class EnigmaTextComparison:
    equivalent: bool = False
    differences: frozenset[str] = frozenset()
    transformations: tuple[str, ...] = ()


def compare_enigma_text(path: str, class_name: str, source_value: Any,
        companion_value: Any, source_leaves: dict[str, tuple[Any, str | None]],
        companion_leaves: dict[str, tuple[Any, str | None]], source_row: dict[str, Any],
        companion_row: dict[str, Any]) -> EnigmaTextComparison:
    """Centralizes semantic comparison and classification of one Enigma text value."""
    normalized_source = normalize_legacy_text_whitespace(source_value)
    normalized_companion = normalize_legacy_text_whitespace(companion_value)
    control_differences = ({"whitespace"}
        if normalized_source != source_value or normalized_companion != companion_value
        else set())
    source_chunks = parse_enigma_chunks(normalized_source, source_row)
    companion_chunks = parse_enigma_chunks(normalized_companion, companion_row)
    if equal_after_synthesized_initial_state(path, class_name, normalized_source,
            normalized_companion, source_row, companion_row):
        return EnigmaTextComparison(
            differences=frozenset(control_differences | {"added font info"}))
    if enigma_chunks_equal(source_chunks, companion_chunks):
        if control_differences:
            return EnigmaTextComparison(differences=frozenset(control_differences))
        return EnigmaTextComparison(True, transformations=(
            "Equivalent Enigma font-state serialization",))

    source_without_time = normalize_enigma_time_inserts(normalized_source)
    companion_without_time = normalize_enigma_time_inserts(normalized_companion)
    if (source_without_time != normalized_source
            or companion_without_time != normalized_companion):
        if enigma_chunks_equal(parse_enigma_chunks(source_without_time, source_row),
                parse_enigma_chunks(companion_without_time, companion_row)):
            if control_differences:
                return EnigmaTextComparison(differences=frozenset(control_differences))
            return EnigmaTextComparison(True, transformations=("Finale-dropped ^time insert",))

    if equal_unformatted_metadata_text(class_name, normalized_source, normalized_companion,
            source_chunks, companion_chunks):
        if control_differences:
            return EnigmaTextComparison(differences=frozenset(control_differences))
        return EnigmaTextComparison(True, transformations=(
            "Equivalent unformatted metadata text",))
    if equal_empty_enigma_text(normalized_source, normalized_companion,
            source_chunks, companion_chunks):
        if control_differences:
            return EnigmaTextComparison(differences=frozenset(control_differences))
        return EnigmaTextComparison(True, transformations=("Equivalent empty Enigma text",))
    if (class_name == "block_texts" and equal_part_name_text(path, normalized_source,
            normalized_companion, source_row, companion_row,
            source_chunks, companion_chunks)):
        if control_differences:
            return EnigmaTextComparison(differences=frozenset(control_differences))
        return EnigmaTextComparison(True, transformations=(
            "Finale-reformatted part-name text",))
    differences = control_differences | enigma_chunk_differences(
        source_chunks, companion_chunks)
    if "text" in differences:
        differences.remove("text")
        if is_empty_companion_partname_template(path, normalized_source, normalized_companion,
                source_leaves, companion_leaves, source_row):
            differences.add("empty part-name template")
        elif (class_name == "block_texts" and is_defaulted_part_name_text(
                path, source_row, companion_row, source_chunks, companion_chunks)):
            differences.add("empty part-name template")
        elif is_whitespace_text_difference(path, normalized_source, normalized_companion,
                source_leaves, companion_leaves, source_row, companion_row,
                source_chunks, companion_chunks):
            differences.add("whitespace")
        elif enigma_text_has_missing_run(source_chunks, companion_chunks):
            differences = control_differences | {"missing run"}
        # Encoding recognition is deliberately last because it transforms every chunk.
        elif finale_wrong_platform_encoding(source_chunks, companion_chunks):
            differences.add("known encoding glitch")
        elif finale_symbol_font_encoding_glitch(source_chunks, companion_chunks):
            differences.add("known encoding glitch")
        elif finale_utf16le_byte_pair_glitch(source_chunks, companion_chunks):
            # Once an 8-bit run has entered Finale's UTF-16 path, later command boundaries,
            # truncation, and formatting state are consequences of the same corruption and
            # are not independently meaningful differences.
            differences = control_differences | {"known encoding glitch"}
        else:
            differences.add("other")
    return EnigmaTextComparison(differences=frozenset(differences or {"other"}))


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


def is_finale_stem_horizontal_correction(_path: str, source_value: Any,
        companion_value: Any, source_leaves: dict[str, tuple[Any, Optional[str]]],
        companion_leaves: dict[str, tuple[Any, Optional[str]]],
        _source_row: dict[str, Any]) -> bool:
    """Finale's font-dependent horizontal correction of the default stem glyph.

    All other fields of the first connection must survive exactly. That both limits the
    exception to the observed one-field transformation and prevents a damaged or misaligned
    connection from being excused merely because its horizontal value happens to match one
    of the three observed companion values.
    """
    if source_value != 0 or companion_value not in {199, 221, 589, 6969}:
        return False
    prefix = "stem_options.stem_connections[0]."
    unchanged_fields = (
        "font_name", "font_id", "symbol", "up_stem_vert", "down_stem_vert",
        "down_stem_horz",
    )
    return all(source_leaves.get(prefix + field, (None, None))[0]
        == companion_leaves.get(prefix + field, (None, None))[0]
        for field in unchanged_fields)


def is_coda_synthesized_stem_width(_path: str, _source_value: Any,
        _companion_value: Any, _source_leaves: dict[str, tuple[Any, Optional[str]]],
        _companion_leaves: dict[str, tuple[Any, Optional[str]]],
        source_row: dict[str, Any]) -> bool:
    """Finale synthesizes a Coda stem width where the source stores no such option.

    The reader's 115 is the pinned baseline, not a recovered Coda value. Finale's upgrader
    independently chooses the upgraded value; that value is not evidence of a legacy field
    that the reader should recover.
    """
    return source_row.get("epoch") == "coda-banner"


def is_coda_synthesized_stem_offset(_path: str, source_value: Any,
        companion_value: Any, _source_leaves: dict[str, tuple[Any, Optional[str]]],
        _companion_leaves: dict[str, tuple[Any, Optional[str]]],
        source_row: dict[str, Any]) -> bool:
    """Finale synthesizes a Coda stem offset where the source stores no such option."""
    return (source_row.get("epoch") == "coda-banner" and source_value == 256
        and companion_value == 128)


def is_coda_text_block_upgrade(path: str, source_value: Any,
        companion_value: Any, _source_leaves: dict[str, tuple[Any, Optional[str]]],
        _companion_leaves: dict[str, tuple[Any, Optional[str]]],
        source_row: dict[str, Any]) -> bool:
    """Finale enables modern TextBlock positioning or shape state during a Coda upgrade."""
    if source_row.get("epoch") != "coda-banner":
        return False
    if path.endswith(".shape_id"):
        return (type(source_value) is int and source_value == 0
            and type(companion_value) is int and companion_value != 0)
    return source_value is False and companion_value is True


def is_baseline_font_definition(path: str, _source_value: Any,
        _companion_value: Any, _source_leaves: dict[str, tuple[Any, Optional[str]]],
        _companion_leaves: dict[str, tuple[Any, Optional[str]]],
        source_row: dict[str, Any]) -> bool:
    """Recognizes a matched definition supplied by the reader's baseline, not the source."""
    match = re.match(
        r"^font_definitions\.definitions\[(cmper=0|normalized_name=([^\]#]+))(?:#\d+)?\]\.",
        path)
    if not match:
        return False
    definitions = (source_row.get("font_definitions") or {}).get("definitions") or []
    if match.group(1) == "cmper=0":
        definition = next((item for item in definitions if item.get("cmper") == 0), None)
    else:
        identity = match.group(2)
        definition = next((item for item in definitions
            if normalize_font_name(item.get("normalized_name")) == identity), None)
    return isinstance(definition, dict) and definition.get("origin") != "legacy-mus"


def is_shape_reclassified_as_other(_path: str, source_value: Any,
        companion_value: Any, _source_leaves: dict[str, tuple[Any, Optional[str]]],
        _companion_leaves: dict[str, tuple[Any, Optional[str]]],
        _source_row: dict[str, Any]) -> bool:
    """Finale may reclassify a specifically typed legacy shape as Other."""
    return source_value != 0 and companion_value == 0


def equal_omitted_zero_insert_font(path: str, source_value: Any,
        companion_value: Any, source_leaves: dict[str, tuple[Any, Optional[str]]],
        companion_leaves: dict[str, tuple[Any, Optional[str]]],
        _source_row: dict[str, Any]) -> bool:
    """Treats an omitted symFont as the stored all-zero no-override sentinel."""
    prefix = path.rsplit(".", 1)[0]
    if path.endswith(".has_font"):
        if source_value is not True or companion_value is not False:
            return False
    elif path.endswith(".normalized_font_name"):
        if not source_value or companion_value != "":
            return False
    else:
        return False
    if any(leaves.get(prefix + field, (None, None))[0] != 0
            for leaves in (source_leaves, companion_leaves)
            for field in (".font_id", ".font_size", ".font_effects")):
        return False
    unchanged = (".present", ".tracking_before", ".tracking_after",
        ".baseline_shift_perc", ".sym_char", ".dangling_font")
    return all(source_leaves.get(prefix + field, (None, None))[0]
        == companion_leaves.get(prefix + field, (None, None))[0]
        for field in unchanged)


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
        "stem-connection-past-terminator — musxdom completes the stem-connection table past the first "
        "symbol-less entry, where this reader stops (see stem_options.cpp)"),
    (r"^stem_options\.stem_connections\[0\]\.up_stem_horz$",
        {"differs"}, {"legacy-mus"}, is_finale_stem_horizontal_correction,
        "stem-horizontal-correction — Finale applied one of the observed font-dependent horizontal corrections "
        "to the default stem glyph while leaving every other connection field unchanged"),
    (r"^(?:stem_options\.stem_connections\[\d+\]\.symbol|"
        r"ss_line_styles\[cmper=\d+\]\.line_char)$",
        {"differs"}, None, is_finale_symbol_glyph_reinterpretation,
        "symbol-glyph-reinterpretation — Finale treated a font that the source describes as ordinary text "
        "as an installation-configured symbol font and preserved the reversibly recovered legacy byte"),
    (r"^stem_options\.stem_width$", {"differs"}, {"finale27-default"},
        is_coda_synthesized_stem_width,
        "coda-stem-width — the Coda source stores no stem-width option: this reader retains the pinned "
        "baseline's 115 while Finale's upgrader synthesizes its own value"),
    (r"^stem_options\.stem_offset$", {"differs"}, {"finale27-default"},
        is_coda_synthesized_stem_offset,
        "coda-stem-offset — the Coda source stores no stem-offset option: this reader retains the pinned "
        "baseline's 256 while Finale's upgrader synthesizes 128"),
    (r"^text_blocks\[cmper=[^\]]+\]\.(?:shape_id|show_shape|new_pos_36|no_expand_single_word)$",
        {"differs"}, None, is_coda_text_block_upgrade,
        "coda-text-block-upgrade — Finale enabled modern TextBlock positioning or shape state "
        "while upgrading a Coda-era document"),
    (r"^shape_defs\[cmper=[^\]]+\]\.shape_type$", {"differs"}, {"legacy-mus"},
        is_shape_reclassified_as_other,
        "shape-reclassified-other — Finale reclassified a specifically typed legacy shape as Other"),
    (r"\bshape_id$", {"differs"}, {"finale27-default"}, None,
        "default-shape-id — a shape_id the source side never recovered -- it is the pinned Finale 27 "
        "baseline's own default reference -- so it names a shape in that baseline's own "
        "numbering, not this specific companion's; shape ids are reassigned per save (see "
        "the cmper instability note on LIST_MATCH_KEY_OVERRIDES), so the two numbers "
        "agreeing was never the right test. What should agree, and isn't checked here yet, "
        "is whether the two referenced shapes are the same shape."),
    (r"^font_definitions\.definitions\[", {"differs"}, None,
        is_baseline_font_definition,
        "baseline-font — the source has no such font definition: this reader and Finale "
        "independently supplied it from their own defaults"),
    (r"^font_definitions\.definitions\[(?:normalized_name=[^\]]+|cmper=0)\]\.charset_value$",
        {"differs"}, None, is_finale_symbol_charset_normalization,
        "symbol-charset — Finale converted the font descriptor to Mac symbol (bank 0, value 4095) "
        "or Windows symbol (bank 1, value 2)"),
    (r"^font_definitions\.definitions\[(?:normalized_name=[^\]]+|cmper=0)\]\.charset_(?:bank|value)$",
        {"differs"}, None, is_finale_platform_charset_normalization,
        "platform-charset-numbering — Finale converted platform-specific charset numbering "
        "that has no semantic significance in musxdom"),
    (r"^lyric_options\.(?:use_smart_hyphens|use_smart_word_extensions)$",
        {"differs"}, {"legacy-behavior"}, is_false_to_true,
        "smart-lyrics-enabled — Finale enabled smart lyric hyphens and word extensions while upgrading the "
        "lyrics and synthesizing the smart shapes that implement them; the source era "
        "had no stored option, so this does not excuse a later explicit false value"),
    (r"^text_options\.inserts\[(?:1|2)\]\.tracking_before$",
        {"differs"}, {"finale27-default"}, is_absent_legacy_insert_default,
        "missing-accidental-insert-default — the source has no accidental-insert record: this reader retains the pinned "
        "Finale 27 baseline while Finale's upgrader synthesizes the older flat and "
        "natural tracking defaults"),
    (r"^lyric_options\.word_ext_connect_styles\.oneEntryEnd\.x$",
        {"differs"}, None, is_pre_connection_table_one_entry_offset,
        "pre-connection-endpoint — before the connection table exists, Finale changes the baseline one-entry "
        "endpoint from 42 to 44 while leaving every other connection-style field intact; "
        "the reader keeps the pinned baseline rather than guessing the upgrade formula"),
    (r"^text_options\.inserts\[\d+\]\.", {"differs"}, None,
        is_finale_17_byte_insert_misconversion,
        "17-byte-accidental-insert — Finale mis-converted the complete 17-byte accidental-insert layout as the later "
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
    font_identity() correctly fails to find it rather than reporting a wrong name.
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


def equal_resolved_font_name(path: str,
        source_leaves: dict[str, tuple[Any, Optional[str]]],
        companion_leaves: dict[str, tuple[Any, Optional[str]]],
        source_row: dict, companion_row: dict) -> bool:
    """Compares every font-name leaf through the report's semantic font identity."""
    if path.startswith("font_definitions.definitions[") and path.endswith(".normalized_name"):
        return fonts_equal(source_row, ("name", source_leaves[path][0]),
            companion_row, ("name", companion_leaves[path][0]))
    if not path.endswith("font_name"):
        return False
    font_id_path = path.removesuffix(".font_name") + ".font_id"
    source_id = source_leaves.get(font_id_path, (None, None))[0]
    companion_id = companion_leaves.get(font_id_path, (None, None))[0]
    source_reference = (("id", source_id) if source_id is not None
        else ("name", source_leaves[path][0]))
    companion_reference = (("id", companion_id) if companion_id is not None
        else ("name", companion_leaves[path][0]))
    return fonts_equal(source_row, source_reference, companion_row, companion_reference)


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

    def remap_from_signatures(source_items: list[dict], companion_items: list[dict],
            source_signature_of, companion_signature_of) -> dict[int, int]:
        """Pairs unchanged same-cmper items first, then pairs remaining signatures.

        Duplicate signatures are common, especially empty instruction/data objects and blank
        shapes. Letting encounter order pair those duplicates can cross two objects that retained
        both their cmper and content, manufacturing a leaf difference in the enclosing ShapeDef.
        A cmper is not identity across a save by itself, but matching cmper *and* content is an
        unambiguous anchor and must be consumed before the unstable-number fallback.
        """
        remap: dict[int, int] = {}
        matched_source: set[int] = set()
        source_by_cmper = {item["cmper"]: item for item in source_items}
        for item in companion_items:
            source_item = source_by_cmper.get(item["cmper"])
            if (source_item is not None
                    and source_signature_of(source_item) == companion_signature_of(item)):
                remap[item["cmper"]] = item["cmper"]
                matched_source.add(item["cmper"])

        available = by_signature(
            [item for item in source_items if item["cmper"] not in matched_source],
            source_signature_of)
        for item in companion_items:
            if item["cmper"] in remap:
                continue
            candidates = available.get(companion_signature_of(item))
            if candidates:
                remap[item["cmper"]] = candidates.pop(0)
        return remap

    def remap_from(source_items: list[dict], companion_items: list[dict], signature_of) -> dict[int, int]:
        return remap_from_signatures(
            source_items, companion_items, signature_of, signature_of)

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
    data_remap = remap_from_signatures(source_buffers, companion_buffers,
        lambda b: source_data_sig[b["cmper"]],
        lambda b: companion_data_sig[b["cmper"]])

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

    source_shape_signature = lambda s: shape_signature(
        s, source_lists_by_cmper, source_data_sig)
    companion_shape_signature = lambda s: shape_signature(
        s, companion_lists_by_cmper, companion_data_sig)

    # Preserve same-cmper, same-content ShapeDefs before matching duplicate signatures by
    # encounter order. A blank shape's signature is (None, None), so without these anchors one
    # blank can be cross-paired with another and acquire the other's harmless stale references.
    shape_remap: dict[int, int] = {}
    matched_source_shapes: set[int] = set()
    source_shapes_by_cmper = {shape["cmper"]: shape for shape in source_shapes}
    for shape in companion_shapes:
        source_shape = source_shapes_by_cmper.get(shape["cmper"])
        if (source_shape is not None
                and source_shape_signature(source_shape) == companion_shape_signature(shape)):
            shape_remap[shape["cmper"]] = shape["cmper"]
            matched_source_shapes.add(shape["cmper"])

    available_shapes = by_signature(
        [shape for shape in source_shapes if shape["cmper"] not in matched_source_shapes],
        source_shape_signature)

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

    wrapped_matches: list[tuple[dict, dict, int]] = []
    for shape in companion_shapes:
        if shape["cmper"] in shape_remap:
            continue
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
        return ("normalized_name", normalize_font_name(item["normalized_name"]))
    return None


def _text_record_key(item: dict) -> tuple[str, str] | None:
    """Returns a report-assigned text identity, then the record's ordinary musx identity."""
    if isinstance(item, dict) and "_report_match_key" in item:
        return ("semantic", str(item["_report_match_key"]))
    return _musx_list_item_key(item)


def _musx_list_item_key(item: Any) -> tuple[str, str] | None:
    """Returns an item's musx identity when the surveyor exposes one.

    A musx array is a collection of keyed objects unless its elements genuinely have no
    identity of their own (for example, an instruction sequence).  Serialization order is
    therefore never a substitute for a comparator, number, incidence tuple, or fixed array
    index.  More-specific tuples must precede their component fields here so an incidence is
    not accidentally folded into another object with the same primary comparator.
    """
    if not isinstance(item, dict):
        return None
    if all(field in item for field in ("cmper1", "cmper2", "inci")):
        return ("identity", ",".join(
            f"{field}={item[field]}" for field in ("cmper1", "cmper2", "inci")))
    if all(field in item for field in ("cmper", "inci")):
        return ("identity", f"cmper={item['cmper']},inci={item['inci']}")
    for field in ("cmper", "number", "index"):
        if field in item:
            return (field, str(item[field]))
    return None


def realign_coda_block_texts(source: dict, companion: dict) -> int:
    """Prunes semantically paired Coda block texts before numbered comparison.

    Coda `HT`/`HS` records have no modern global BlockText number. The reader allocates
    numbers while walking those arrays, whereas Finale allocates from a larger upgraded
    pool that can include expression-related block texts first. Pairing by number would
    therefore compare unrelated strings. Exact chunk signatures use the report's one
    Enigma/font equivalence implementation; duplicate signatures pair one-for-one. Residual
    records receive side-specific report keys so coincidentally equal generated numbers do
    not pair them again.
    """
    if source.get("epoch") != "coda-banner":
        return 0
    source_items = source.get("block_texts")
    companion_items = companion.get("block_texts")
    if not isinstance(source_items, list) or not isinstance(companion_items, list):
        return 0

    companion_by_signature: dict[tuple, list[int]] = {}
    for index, item in enumerate(companion_items):
        if not isinstance(item, dict):
            continue
        signature = enigma_chunks_signature(item.get("text"), companion)
        if signature is not None:
            companion_by_signature.setdefault(signature, []).append(index)

    matched_source: set[int] = set()
    matched_companion: set[int] = set()
    for index, item in enumerate(source_items):
        if not isinstance(item, dict):
            continue
        signature = enigma_chunks_signature(item.get("text"), source)
        candidates = companion_by_signature.get(signature) if signature is not None else None
        if candidates:
            matched_source.add(index)
            matched_companion.add(candidates.pop())

    source_remaining = [dict(item) if isinstance(item, dict) else item
        for index, item in enumerate(source_items) if index not in matched_source]
    companion_remaining = [dict(item) if isinstance(item, dict) else item
        for index, item in enumerate(companion_items) if index not in matched_companion]
    for index, item in enumerate(source_remaining):
        if isinstance(item, dict):
            item["_report_match_key"] = f"source-{index}"
    for index, item in enumerate(companion_remaining):
        if isinstance(item, dict):
            item["_report_match_key"] = f"companion-{index}"
    source["block_texts"] = source_remaining
    companion["block_texts"] = companion_remaining
    return len(matched_source)


# list_path (the dotted path to the *list itself*, e.g. "font_definitions.definitions") ->
# a function from one item to its (field, key) match identity, overriding the general musx
# identity below for classes whose ordinary comparator is not stable across documents.
LIST_MATCH_KEY_OVERRIDES: dict[str, Any] = {
    "font_definitions.definitions": _font_definition_key,
    **{class_name: _text_record_key for class_name, pool in SURVEY_CLASS_POOLS.items()
        if pool == "texts"},
}


def list_path_segments(items: list, list_path: str) -> list[str]:
    """The path segment for each item in `items` (the list found at `list_path`): keyed by
    `LIST_MATCH_KEY_OVERRIDES[list_path]` when present, otherwise by the musx identity
    selected in `_musx_list_item_key()`. Positional indexing is reserved for elements that
    genuinely expose no identity. Neither side is guaranteed to serialize records in the
    same order or count, so matching objects by position would misalign every record after
    one side adds or removes an item. A repeated key within one list is suffixed `#2`, `#3`,
    ... in encounter order rather than silently collapsed onto the first, so two same-named
    fonts (say) still each get their own path -- imperfectly, since encounter order is the
    only thing pairing them, but not silently.
    """
    override = LIST_MATCH_KEY_OVERRIDES.get(list_path)
    keys: list[tuple[str, str]] | None = None
    if override is not None:
        resolved = [override(item) for item in items]
        if items and all(key is not None for key in resolved):
            keys = resolved
    if keys is None:
        resolved = [_musx_list_item_key(item) for item in items]
        if not items or not all(key is not None for key in resolved):
            return [f"[{index}]" for index in range(len(items))]
        keys = resolved
    seen: Counter[tuple[str, str]] = Counter()
    segments = []
    for field, key in keys:
        seen[(field, key)] += 1
        suffix = "" if seen[(field, key)] == 1 else f"#{seen[(field, key)]}"
        if field == "index":
            segments.append(f"[{key}{suffix}]")
        elif field == "identity":
            segments.append(f"[{key}{suffix}]")
        else:
            segments.append(f"[{field}={key}{suffix}]")
    return segments


@cache
def snake_to_camel(name: str) -> str:
    first, *rest = name.split("_")
    return first + "".join(word.capitalize() for word in rest)


def leaf_paths(value: Any, prefix: str = "", origin: str | None = None,
        include_origins: bool = True) -> Iterator[tuple[str, Any, str | None]]:
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
            child_origin = None
            if include_origins:
                child_origin = (value.get(f"origin_{snake_to_camel(key)}")
                    or value.get(f"{key}_origin"))
            yield from leaf_paths(sub, child_prefix, child_origin, include_origins)
    elif isinstance(value, list):
        for segment, item in zip(list_path_segments(value, prefix), value):
            yield from leaf_paths(item, f"{prefix}{segment}", include_origins=include_origins)
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


def expected_difference_slug(label: str) -> str:
    """Returns the stable short identifier from a rule's explanatory label."""
    return label.split(" — ", 1)[0]


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


def record_expected_difference(stats: ClassStats, transformations: Counter[str],
        label: str) -> None:
    """Records one expected difference in both the class totals and rule tally."""
    stats.expected_diff += 1
    transformations[f"Expected diff: {label}"] += 1


FINALE_TEXT_BLOCK_RENUMBERING = (
    "finale-text-block-renumbering — Finale reused or renumbered a TextBlock comparator")
TRANSIENT_TEXT_BLOCK = (
    "transient-text-block — Finale uses TextBlock comparators above 65000 for transient records")
LEGACY_PAGE_PARITY_TEXT = (
    "legacy-page-parity-text — Finale replaced opposing-page TextBlocks with modern page-number positioning")
NONMATCHING_TEXT_BLOCK_TEXT_DIFFERENCES = frozenset({"other", "missing run"})


def enigma_text_is_page_insert_only(value: Any, row: dict[str, Any]) -> bool:
    """Whether formatting commands surround exactly one page-number insert and no other text."""
    chunks = parse_enigma_chunks(value, row)
    return (chunks is not None
        and re.fullmatch(r"\^page\([^)]*\)", "".join(chunk.text for chunk in chunks)) is not None)


def text_block_referent_comparisons(source_row: dict[str, Any],
        companion_row: dict[str, Any]) -> dict[str, str]:
    """Classifies raw-text referents for differing same-cmper TextBlocks.

    A `matching` result means the existing Enigma comparison found neither an unclassified
    text difference nor a missing run. A `renumbered` result means it found `other` or
    `missing run`, either of which means the referenced content is not semantically equal.
    An unresolved referent normally has no result. When Finale also changes the text family
    and text id, the combined change identifies reclassification and renumbering even though
    one side's old-family referent cannot be compared.
    """
    source_blocks = {item.get("cmper"): item for item in source_row.get("text_blocks") or []}
    companion_blocks = {
        item.get("cmper"): item for item in companion_row.get("text_blocks") or []}
    raw_texts: dict[tuple[int, str], tuple[dict[int, Any],
        dict[str, tuple[Any, str | None]]]] = {}

    def raw_text(row: dict[str, Any], side: int, block: dict[str, Any]) -> tuple[
            str, str, Any, dict[str, tuple[Any, str | None]]] | None:
        pool = {0: "block_texts", 1: "expression_texts"}.get(block.get("text_type"))
        text_id = block.get("text_id")
        if pool is None or not isinstance(text_id, int):
            return None
        cache_key = (side, pool)
        if cache_key not in raw_texts:
            values = {item.get("number"): item.get("text")
                for item in row.get(pool) or [] if isinstance(item.get("number"), int)}
            leaves = {path: (value, origin) for path, value, origin in leaf_paths(
                row.get(pool), pool, include_origins=False)}
            raw_texts[cache_key] = (values, leaves)
        values, leaves = raw_texts[cache_key]
        if text_id not in values:
            return None
        return pool, f"{pool}[number={text_id}].text", values[text_id], leaves

    results: dict[str, str] = {}
    for cmper in source_blocks.keys() & companion_blocks.keys():
        source_block = source_blocks[cmper]
        companion_block = companion_blocks[cmper]
        fields = ((set(source_block) | set(companion_block))
            - {key for key in set(source_block) | set(companion_block)
                if key.startswith("origin_")})
        if all(source_block.get(field) == companion_block.get(field) for field in fields):
            continue
        source_text = raw_text(source_row, 0, source_block)
        companion_text = raw_text(companion_row, 1, companion_block)
        if source_text is None or companion_text is None:
            prefix = f"{TEXT_BLOCKS_SURVEYOR}[cmper={cmper}]"
            source_zero_without_text = source_block.get("text_id") == 0 and source_text is None
            companion_zero_without_text = (
                companion_block.get("text_id") == 0 and companion_text is None)
            if ((source_zero_without_text and companion_text is not None)
                    or (companion_zero_without_text and source_text is not None)):
                results[prefix] = "renumbered"
            elif (source_block.get("text_id") != companion_block.get("text_id")
                    and source_block.get("text_type") != companion_block.get("text_type")
                    and (source_text is not None or companion_text is not None)):
                results[prefix] = "renumbered"
            continue
        source_pool, source_path, source_value, source_leaves = source_text
        companion_pool, _companion_path, companion_value, companion_leaves = companion_text
        comparison = compare_enigma_text(source_path,
            source_pool if source_pool == companion_pool else TEXT_BLOCKS_SURVEYOR,
            source_value, companion_value, source_leaves, companion_leaves,
            source_row, companion_row)
        prefix = f"{TEXT_BLOCKS_SURVEYOR}[cmper={cmper}]"
        if NONMATCHING_TEXT_BLOCK_TEXT_DIFFERENCES & comparison.differences:
            results[prefix] = "renumbered"
        elif (enigma_text_is_page_insert_only(source_value, source_row)
                and enigma_text_is_page_insert_only(companion_value, companion_row)):
            results[prefix] = "matching-page-only"
        else:
            results[prefix] = "matching"
    return results


def compare_row(source: dict, companion: dict) -> tuple[
        dict[str, ClassStats], list[tuple[str, str, Category, Any, Any]],
        list[tuple[str, str, str]], list[tuple[str, str, Category, Any, Any, str]], Counter[str]]:
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
    textual: list[tuple[str, str, Category, Any, Any, str]] = []
    # TextBlock referents use the original raw-text ids. Capture their comparison before
    # Coda block-text alignment replaces the text pool with report-keyed residual records.
    text_block_comparisons = text_block_referent_comparisons(source, companion)
    coda_block_text_matches = realign_coda_block_texts(source, companion)
    # Must run before shape_set_font_paths(source) is used below and before any
    # leaf_paths() call over companion's shape_defs/shape_instruction_lists/shape_data:
    # it mutates companion's own cmpers in place to match source's numbering.
    wrapper_count = realign_shape_cmpers(source, companion)
    transformations: Counter[str] = Counter()
    transformations["Finale-added StartObject wrapper"] = wrapper_count
    transformations["Semantically paired Coda block text"] = coda_block_text_matches
    source_font_id_paths = shape_set_font_paths(source)
    classes = ((set(source) | set(companion)) - METADATA_KEYS
        - COMPARISON_EXCLUDED_CLASSES)
    for class_name in classes:
        class_stats = stats.setdefault(class_name, ClassStats())
        if class_name == "block_texts":
            class_stats.same += coda_block_text_matches
        # Seeded with class_name, not "", so every path leaf_paths() yields (and every
        # LIST_MATCH_KEY_OVERRIDES lookup inside it) is already fully qualified -- matching
        # what full_path needs below without a second, easy-to-desync prefixing step here.
        # {path: (value, source_origin)}; companion_leaves' origin slot is always None
        # (see leaf_paths()) and is not read below.
        source_leaves = {path: (value, origin)
            for path, value, origin in leaf_paths(source.get(class_name), class_name)}
        companion_leaves = {path: (value, origin)
            for path, value, origin in leaf_paths(
                companion.get(class_name), class_name, include_origins=False)}
        source_paths = set(source_leaves)
        companion_paths = set(companion_leaves)
        referent_comparisons = (text_block_comparisons
            if class_name == TEXT_BLOCKS_SURVEYOR else {})
        recorded_renumberings: set[str] = set()
        for full_path in source_paths | companion_paths:
            first_list_end = full_path.find("]")
            object_prefix = full_path[:first_list_end + 1] if first_list_end >= 0 else ""
            in_source = full_path in source_leaves
            in_companion = full_path in companion_leaves
            if in_source and in_companion:
                if source_leaves[full_path][0] == companion_leaves[full_path][0]:
                    class_stats.same += 1
                    continue
                if (full_path == object_prefix + ".text_id"
                        and referent_comparisons.get(object_prefix)
                            in {"matching", "matching-page-only"}):
                    class_stats.same += 1
                    transformations["Equivalent TextBlock raw-text referent"] += 1
                    continue
                if class_name in ENIGMA_TEXT_SURVEYORS and full_path.endswith(".text"):
                    source_value = source_leaves[full_path][0]
                    companion_value = companion_leaves[full_path][0]
                    comparison = compare_enigma_text(full_path, class_name,
                        source_value, companion_value, source_leaves, companion_leaves,
                        source, companion)
                    if comparison.equivalent:
                        class_stats.same += 1
                        transformations.update(comparison.transformations)
                    else:
                        for kind in sorted(comparison.differences):
                            textual.append((class_name, full_path, "differs",
                                source_value, companion_value, kind))
                    continue
                if equal_omitted_zero_insert_font(full_path,
                        source_leaves[full_path][0], companion_leaves[full_path][0],
                        source_leaves, companion_leaves, source):
                    class_stats.same += 1
                    continue
                if equal_resolved_font_name(full_path, source_leaves, companion_leaves,
                        source, companion):
                    class_stats.same += 1
                    transformations["Equivalent normalized font-name spelling"] += 1
                    continue
                if equal_symbol_font_platform_spelling(
                        full_path, source_leaves, companion_leaves):
                    class_stats.same += 1
                    if full_path.endswith(".charset_bank"):
                        transformations[
                            "Equivalent Windows/Mac symbol font descriptor"] += 1
                    continue
                category = "differs"
            elif in_source:
                category = "reader_only"
            else:
                category = "companion_only"

            if category == "differs" and full_path in source_font_id_paths:
                source_reference = ("id", source_leaves[full_path][0])
                companion_reference = ("id", companion_leaves[full_path][0])
                source_name = font_identity(source, source_reference)
                companion_name = font_identity(companion, companion_reference)
                if fonts_equal(source, source_reference, companion, companion_reference):
                    class_stats.same += 1
                    continue
                record_expected_difference(class_stats, transformations,
                    "setfont-font-substitution — Finale remapped a shape SetFont instruction "
                    "to a different document-local font id")
                substitutions.append((full_path, source_name or "?", companion_name or "?"))
                continue

            source_value = source_leaves[full_path][0] if in_source else None
            companion_value = companion_leaves[full_path][0] if in_companion else None
            source_origin = source_leaves[full_path][1] if in_source else None
            label = classify_difference(full_path, category, source_origin,
                source_value, companion_value, source_leaves, companion_leaves, source)
            if label is not None:
                record_expected_difference(class_stats, transformations, label)
                continue

            if (category == "differs" and full_path == object_prefix + ".justify"
                    and referent_comparisons.get(object_prefix) == "matching-page-only"
                    and {source_value, companion_value} == {0, 2}):
                record_expected_difference(class_stats, transformations,
                    LEGACY_PAGE_PARITY_TEXT)
                continue

            if (category == "differs"
                    and referent_comparisons.get(object_prefix) == "renumbered"):
                class_stats.expected_diff += 1
                if object_prefix not in recorded_renumberings:
                    transformations[f"Expected diff: {FINALE_TEXT_BLOCK_RENUMBERING}"] += 1
                    recorded_renumberings.add(object_prefix)
                continue

            if class_name == TEXT_BLOCKS_SURVEYOR and object_prefix:
                cmper_text = object_prefix.removeprefix(
                    f"{TEXT_BLOCKS_SURVEYOR}[cmper=").removesuffix("]")
                if cmper_text.isdigit() and int(cmper_text) > 65000:
                    record_expected_difference(class_stats, transformations,
                        TRANSIENT_TEXT_BLOCK)
                    continue

            # This is deliberately the final differing-value comparison. Resolving every
            # font run and proving a whole-string legacy-code-page conversion is expensive;
            # exact equality, the normal text transformations, SetFont handling, and all
            # ordinary expected-difference rules must get the first chance to settle it.
            if category == "differs":
                class_stats.unexpected_diff += 1
            elif category == "reader_only":
                class_stats.reader_only += 1
            else:
                class_stats.companion_only += 1
            unexpected.append((class_name, full_path, category, source_value, companion_value))
    return stats, unexpected, substitutions, textual, transformations


def read_rows(path: Path, show_progress: bool = False) -> Iterator[dict]:
    total_bytes = path.stat().st_size
    consumed_bytes = 0
    row_count = 0
    companion_count = 0
    started = time.monotonic()
    next_progress = started + PROGRESS_INTERVAL_SECONDS
    stderr_is_terminal = sys.stderr.isatty()

    def display_progress(done: bool = False) -> None:
        elapsed = time.monotonic() - started
        percent = 100.0 if total_bytes == 0 else consumed_bytes * 100.0 / total_bytes
        rate = 0.0 if elapsed == 0 else row_count / elapsed
        message = (f"Processed {row_count} row(s), {companion_count} companion(s), "
            f"{percent:.1f}% of input in {elapsed:.1f}s ({rate:.1f} rows/s)")
        print(message, file=sys.stderr,
            end="\n" if done or not stderr_is_terminal else "\r", flush=True)

    with path.open("rb") as handle:
        for line in handle:
            consumed_bytes += len(line)
            if not line.strip():
                continue
            row = json.loads(line)
            row_count += 1
            if row.get("companion"):
                companion_count += 1
            yield row
            if show_progress:
                now = time.monotonic()
                if row_count == 1 or now >= next_progress:
                    display_progress()
                    next_progress = now + PROGRESS_INTERVAL_SECONDS
    if show_progress:
        display_progress(done=True)


def table_widths(headers: list[str], rows: Iterable[list[str]]) -> list[int]:
    widths = [len(h) for h in headers]
    for row in rows:
        for index, cell in enumerate(row):
            widths[index] = max(widths[index], len(cell))
    return widths


def truncate_display(value: str, width: int) -> str:
    """Fits one display cell to @p width Unicode characters, including the ellipsis."""
    return value if len(value) <= width else value[:width - 1] + "…"


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


def report_recovery_coverage(rows: Iterable[dict], max_unexpected: int) -> bool:
    row_count = 0
    status_counts: Counter[str] = Counter()
    epoch_counts: Counter[str] = Counter()
    failure_counts: Counter[str] = Counter()
    failure_examples: dict[str, str] = {}
    companion_count = 0
    companion_status: Counter[str] = Counter()
    totals: dict[str, ClassStats] = {}
    unexpected_diff_count = 0
    unexpected_diff_examples: list[tuple[str, str, str, Any, Any]] = []
    textual_counts: Counter[tuple[str, str]] = Counter()
    textual_classes: set[str] = set(ENIGMA_TEXT_SURVEYORS)
    missing_run_count = 0
    missing_run_examples: list[tuple[str, str, str, Any, Any, str]] = []
    other_text_count = 0
    other_text_examples: list[tuple[str, str, str, Any, Any, str]] = []
    font_substitutions: Counter[tuple[str, str]] = Counter()
    transformations: Counter[str] = Counter()
    for row in rows:
        row_count += 1
        status = row.get("status", "?")
        status_counts[status] += 1
        if status == "ok":
            epoch_counts[row.get("epoch", "-")] += 1
        elif status == "error":
            message = row.get("error", "")
            failure_counts[message] += 1
            failure_examples.setdefault(message, row.get("corpus_id", "?"))

        companion = row.get("companion")
        if not companion:
            continue
        companion_count += 1
        companion_status[companion.get("status", "?")] += 1
        if companion.get("status") != "ok":
            continue
        stats, unexpected, substitutions, textual, row_transformations = compare_row(row, companion)
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
                unexpected_diff_count += 1
                if len(unexpected_diff_examples) < max_unexpected:
                    unexpected_diff_examples.append(
                        (row.get("corpus_id", "?"), class_name, path,
                            source_value, companion_value))
        for class_name, path, category, source_value, companion_value, kind in textual:
            textual_counts[(class_name, kind)] += 1
            textual_classes.add(class_name)
            item = (row.get("corpus_id", "?"), class_name, path,
                source_value, companion_value, kind)
            if kind == "missing run":
                missing_run_count += 1
                if len(missing_run_examples) < max_unexpected:
                    missing_run_examples.append(item)
            elif kind == "other":
                other_text_count += 1
                if len(other_text_examples) < max_unexpected:
                    other_text_examples.append(item)
        for _path, source_name, companion_name in substitutions:
            font_substitutions[(source_name, companion_name)] += 1

    if row_count == 0:
        return False

    print(f"\n{row_count} document(s): "
        f"{status_counts.get('ok', 0)} ok, {status_counts.get('error', 0)} error")
    print_table("Epoch (ok documents)", ["epoch", "count"],
        ([epoch, str(count)] for epoch, count in epoch_counts.most_common()))
    print_table("Failure reasons", ["count", "example", "message"],
        ([str(count), failure_examples[message], message]
            for message, count in failure_counts.most_common()))

    if companion_count == 0:
        print("\nNo rows carried a companion (corpus declared no #companion: convention, "
            "or every source failed to import).")
        return True

    print(f"\n{companion_count} row(s) with a companion: "
        f"{companion_status.get('ok', 0)} ok, {companion_status.get('error', 0)} error")

    print_table("SetFont substitutions (also counted as expected differences)",
        ["source font", "companion font", "count"],
        ([source_name, companion_name, str(count)]
            for (source_name, companion_name), count in font_substitutions.most_common()))

    print_table("Recognized companion transformations",
        ["transformation", "count"],
        ([name, str(count)] for name, count in transformations.most_common()
            if count and not name.startswith("Expected diff: ")))
    expected_rows = [
        [expected_difference_slug(name.removeprefix("Expected diff: ")), str(count)]
        for name, count in transformations.most_common()
        if count and name.startswith("Expected diff: ")]
    expected_rows.append(["TOTAL", str(sum(int(row[1]) for row in expected_rows))])
    print_table("Expected differences", ["rule", "count"], expected_rows)

    pool_headers = ["class", "same", "expected-diff", "unexpected-diff",
        "reader-only", "companion-only", "total"]
    pool_order = ("options", "others", "details", "texts", "unclassified")
    pool_rows: dict[str, list[list[str]]] = {}
    for pool in pool_order:
        class_rows = [
            [name, str(stats.same), str(stats.expected_diff), str(stats.unexpected_diff),
                str(stats.reader_only),
                str(stats.companion_only), str(stats.total())]
            for name, stats in sorted(totals.items())
            if SURVEY_CLASS_POOLS.get(name, "unclassified") == pool]
        if class_rows:
            pool_rows[pool] = class_rows + [[
                "TOTAL",
                *[str(sum(int(row[index]) for row in class_rows))
                    for index in range(1, len(pool_headers))],
            ]]
        else:
            pool_rows[pool] = []
    shared_pool_widths = table_widths(pool_headers,
        (row for rows_for_pool in pool_rows.values() for row in rows_for_pool))
    shared_pool_widths[1] = max(shared_pool_widths[1], 10)

    printed_pool = False
    print("\nAll companion-comparison columns count leaves.")
    for pool in pool_order:
        if not pool_rows[pool]:
            continue
        if printed_pool:
            print("\n" + "-" * 80)
        print_table(f"{pool} pool companion comparison",
            pool_headers, pool_rows[pool], shared_pool_widths)
        printed_pool = True

    grand_total = [
        "ALL POOLS",
        *[str(sum(getattr(stats, field) for stats in totals.values()))
            for field in ("same", "expected_diff", "unexpected_diff",
                "reader_only", "companion_only")],
        str(sum(stats.total() for stats in totals.values())),
    ]
    print()
    print("  ".join(cell.ljust(shared_pool_widths[index])
        for index, cell in enumerate(grand_total)))

    textual_kinds = ("known encoding glitch", "whitespace", "font", "size", "effects",
        "added font info", "empty part-name template", "missing run", "other")
    textual_rows = [
        [class_name, *[str(textual_counts[(class_name, kind)]) for kind in textual_kinds],
            str(sum(textual_counts[(class_name, kind)] for kind in textual_kinds))]
        for class_name in sorted(textual_classes)]
    textual_rows.append([
        "TOTAL",
        *[str(sum(int(row[index]) for row in textual_rows))
            for index in range(1, len(textual_kinds) + 2)],
    ])
    print_table("Enigma-text difference findings (one text may contribute more than one)",
        ["text type", "encoding", "whitespace", "font", "size", "effects",
            "added font info", "empty part-name", "missing run", "other", "total"],
        textual_rows)
    if missing_run_count:
        shown = missing_run_examples
        print_table(
            f"Missing-run Enigma-text differences (first {len(shown)} of "
            f"{missing_run_count})",
            ["kind", "corpus_id", "path", "source", "companion"],
            ([kind, corpus_id, full_path,
                truncate_display(json.dumps(source_value, ensure_ascii=False),
                    UNEXPECTED_VALUE_DISPLAY_WIDTH),
                truncate_display(json.dumps(companion_value, ensure_ascii=False),
                    UNEXPECTED_VALUE_DISPLAY_WIDTH)]
                for corpus_id, _class_name, full_path, source_value, companion_value, kind
                in shown))
    if other_text_count:
        shown = other_text_examples
        print_table(
            f"Unclassified Enigma-text differences (first {len(shown)} of "
            f"{other_text_count})",
            ["kind", "corpus_id", "path", "source", "companion"],
            ([kind, corpus_id, full_path,
                truncate_display(json.dumps(source_value, ensure_ascii=False),
                    UNEXPECTED_VALUE_DISPLAY_WIDTH),
                truncate_display(json.dumps(companion_value, ensure_ascii=False),
                    UNEXPECTED_VALUE_DISPLAY_WIDTH)]
                for corpus_id, _class_name, full_path, source_value, companion_value, kind
                in shown))

    if unexpected_diff_count:
        shown = unexpected_diff_examples
        print_table(
            f"Non-text unexpected differences (first {len(shown)} of "
            f"{unexpected_diff_count})",
            ["corpus_id", "path", "source", "companion"],
            ([corpus_id, full_path,
                truncate_display(json.dumps(source_value, ensure_ascii=False),
                    UNEXPECTED_VALUE_DISPLAY_WIDTH),
                truncate_display(json.dumps(companion_value, ensure_ascii=False),
                    UNEXPECTED_VALUE_DISPLAY_WIDTH)]
                for corpus_id, _class_name, full_path, source_value, companion_value in shown))
    return True


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("jsonl", type=Path, help="recovery_coverage_probe output")
    parser.add_argument("--max-unexpected", type=int, default=80,
        help="cap on unexpected-difference rows printed (default: 80)")
    parser.add_argument("--progress", action="store_true",
        help="write input progress to stderr after the first row and every 1.9 seconds")
    args = parser.parse_args()

    if not report_recovery_coverage(
            read_rows(args.jsonl, args.progress), args.max_unexpected):
        print("no rows in input", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
