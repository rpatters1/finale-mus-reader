#!/usr/bin/env python3
# Copyright (c) 2026 Robert G. Patterson
# SPDX-License-Identifier: MIT

"""Aggregate options_coverage_probe observations into an epoch coverage report.

Covers, against companions, **every class the reader recovers**: ClefOptions, FontOptions,
LyricOptions, MultimeasureRestOptions, MusicSpacingOptions, StemOptions, TextOptions,
FontDefinition, LayerAttributes, PageGraphicAssign, ShapeGraphicAssign, ShapeDef,
MeasureGraphicAssign, SmartShapeCustomLine, and all eight text classes -- twenty-two in all,
matching the importer
registry in src/import/support/legacy_mapping.cpp one for one. A full regression is only as wide
as this list, so adding an importer means adding it here.

Some differences a run reports are intended and are not regressions. The largest are:

  TextOptions.symbolInserts.*   Finale 3.7-2000 is mis-converted by Finale 27, which reads an
                                17-byte element as the later 18-byte one. The reader reads the
                                era's own stride and therefore disagrees on nearly every field.
  StemOptions.connection        `past-terminator` counts elements Finale 27 carries beyond the
                                first symbol-less one, where the reader stops.
  BlockText.record              `companion-only` is dominated by the score's part name. Where
                                the source's text pool holds that block the reader recovers it
                                like any other, which is correct; where it does not, the name
                                comes from PartDefinition, which the reader does not yet import.
                                A coverage gap rather than an intended difference, and it closes
                                when PartDefinition does. `reader-only` is a block for a staff
                                the converted document no longer has.
  ExpressionText.record         `companion-only` in the fixed-row and early DCL eras is the `DT`
                                text whose recovery is deferred until TextExpressionDef.
  *.record font-spelling-only   the reader writes a font under the name the source defines; the
                                companion writes whichever spelling Finale 27 chose.
  LyricOptions.use_smart_*      smart hyphens and smart word extensions are implemented as smart
                                shapes. Finale's upgrade manufactures those and switches the
                                options on; this reader does not manufacture them, so it reports
                                false rather than claim a rendering nothing can draw.
  default-omitted-by-companion  a companion omits an element holding musxdom's own default,
                                which musxdom then fills in on the reader's side. Both mean the
                                default; only a source-derived value can disagree with silence.

A `differs` whose origin is `finale27-default` is not a reader error either: it says the pinned
baseline disagrees with the companion for a field the source never stated.

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
    ("text_options", "TextOptions"),
    ("font_definitions", "FontDefinition"),
]

# The text classes are compared against companions but are deliberately **not** in CLASSES, so
# the acceptance rule does not apply to them. That rule asks that no epoch be left entirely
# uncovered for a class under study, which is meaningful for options because every document has
# options: a document with none recovered is a reader failure. A document with no bookmark, no
# lyric and no smart shape is an ordinary document, so the same rule would fail on absence that
# means nothing. Texts are judged by the companion comparison below, where a record the reader
# missed shows up as companion-only.

# StemOptions: probe key, companion element. musxdom's XML names differ from its member names
# in two places -- shortStemLength is <stem2> and stemOffset is <stemLift>. An absent element
# means the field is 0 or false.
STEM_NUMERIC = [
    ("half_stem_length", "halfStemLength"),
    ("stem_length", "stemLength"),
    ("short_stem_length", "stem2"),
    ("rev_stem_adj", "revStemAdj"),
    ("stem_width", "stemWidth"),
    ("stem_offset", "stemLift"),
]
STEM_BOOLEAN = [
    ("use_stem_connections", "useStemConnections"),
    ("no_reverse_stems", "noReverseStems"),
]
# The connection's font is <font>, not <fontID>: musxdom's StemConnection maps fontId to that
# node. An absent element means zero, which is the document's default music font.
STEM_CONNECT_FIELDS = [
    ("font_id", "font"),
    ("symbol", "symbol"),
    ("up_stem_vert", "upStemVert"),
    ("down_stem_vert", "downStemVert"),
    ("up_stem_horz", "upStemHorz"),
    ("down_stem_horz", "downStemHorz"),
]

SPACING_NUMERIC = [
    ("min_width", "minWidth"),
    ("max_width", "maxWidth"),
    ("min_distance", "minDistance"),
    ("min_dist_tied_notes", "minDistTiedNotes"),
]

# LyricOptions: probe key, companion element. An absent element means 0 or false.
LYRIC_NUMERIC = [
    ("hyphen_char", "hyphenChar"),
    ("max_hyphen_separation", "maxHyphenSeparation"),
    ("word_ext_vert_offset", "wordExtVertOffset"),
    ("word_ext_horz_offset", "wordExtHorzOffset"),
    ("word_ext_line_width", "wordExtLineWidth"),
    ("word_ext_min_length", "wordExtMinLength"),
]
LYRIC_BOOLEAN = [
    ("use_smart_word_extensions", "useSmartWordExtensions"),
    ("use_smart_hyphens", "useSmartHyphens"),
    ("use_alt_hyphen_font", "useAltHyphenFont"),
    ("word_ext_need_underscore", "wordExtNeedUnderscore"),
    ("word_ext_offset_to_notehead", "wordExtOffsetToNotehead"),
    ("lyric_use_edge_punctuation", "lyricUseEdgePunctuation"),
    ("show_auto_numbers_verses", "showAutoNumbersOnVerses"),
    ("show_auto_numbers_choruses", "showAutoNumbersOnChoruses"),
    ("show_auto_numbers_sections", "showAutoNumbersOnSections"),
]
# The companion writes an enum name; the probe writes musxdom's ordinal. An absent element is
# the enum's first value, which is 0 for both of these.
LYRIC_ENUMS = [
    ("smart_hyphen_start", "smartHyphenStart",
     {"always": 0, "sometimes": 1, "never": 2}),
    ("lyric_auto_num_type", "lyricAutoNumType", {"none": 0, "align": 1}),
]
LYRIC_TEXT = [("punctuation_to_ignore", "lyricPunctuationToIgnore")]

# The four syllable positions and the nine word-extension connection styles, both keyed by the
# type name musxdom writes as an attribute rather than by position.
ALIGN_JUSTIFY = {"left": 0, "right": 1, "center": 2}
WORD_EXT_CONNECT_INDEX = {
    "lyricRightBottom": 0, "headRightLyrBaseline": 1, "systemLeft": 2,
    "systemRight": 3, "dotRightLyrBaseline": 4, "durationLyrBaseline": 5,
}

# The graphic assignments. The reader recovers the whole stored tuple; these are the members
# whose companion spelling is a plain number and so can be compared without translating an
# enumeration. Note fDescID, whose companion spelling capitalizes differently from the member.
MEAS_GRAPHIC_FIELDS = [
    ("left", "left"),
    ("bottom", "bottom"),
    ("width", "width"),
    ("height", "height"),
    ("f_desc_id", "fDescID"),
    ("orig_width", "origWidth"),
    ("orig_height", "origHeight"),
]

GRAPHIC_FIELDS = [
    ("left", "left"),
    ("bottom", "bottom"),
    ("width", "width"),
    ("height", "height"),
    ("f_desc_id", "fDescID"),
]

# The text classes: probe key, companion element, report label. A text is compared by its
# characters, so there is no field list -- the whole string is the value.
TEXT_CLASSES = [
    ("block_texts", "blockText", "BlockText"),
    ("bookmark_texts", "bookmarkText", "BookmarkText"),
    ("expression_texts", "expression", "ExpressionText"),
    ("lyrics_choruses", "chorus", "LyricsChorus"),
    ("lyrics_sections", "section", "LyricsSection"),
    ("lyrics_verses", "verse", "LyricsVerse"),
    ("smart_shape_texts", "smartShapeText", "SmartShapeText"),
]

# A font command is normalized away before two texts are compared. The reader writes the font
# under the name the source's own definition carries; the companion writes whichever spelling
# Finale 27 chose, which for the same document can be `^fontid(n)`, `^font(Name,charset)` or a
# categorized command, and whose comparator is not stable across re-exports because Finale
# matches fonts against those installed on the upgrading machine. Comparing the spellings would
# manufacture a disagreement on nearly every record; comparing what is left of the text does not.
FONT_COMMAND = re.compile(r"\^(?:font|fontid|Font|fontMus|fontTxt|fontNum)\([^)]*\)")


def unescape_xml(value):
    """The five predefined entities. musxdom escapes nothing else in these fields."""
    for entity, ch in (("&lt;", "<"), ("&gt;", ">"), ("&quot;", '"'),
                       ("&apos;", "'"), ("&amp;", "&")):
        value = value.replace(entity, ch)
    return value


def normalize_text(value):
    return FONT_COMMAND.sub("<F>", value or "")

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

# TextOptions: probe key, companion element, musxdom member. The element names are musxdom's
# own XML mapping, which for this class happens to equal the member names. An absent element
# means the field is 0, false, or the enum's first value.
TEXT_NUMERIC = [
    ("tab_spaces", "tabSpaces", "tabSpaces"),
    ("text_tracking", "textTracking", "textTracking"),
    ("text_baseline_shift", "textBaselineShift", "textBaselineShift"),
    ("text_superscript", "textSuperscript", "textSuperscript"),
    ("text_page_offset", "textPageOffset", "textPageOffset"),
]
TEXT_BOOLEAN = [
    ("show_time_seconds", "showTimeSeconds", "showTimeSeconds"),
    ("text_word_wrap", "textWordWrap", "textWordWrap"),
    ("text_expand_single_word", "textExpandSingleWord", "textExpandSingleWord"),
    ("text_is_edge_aligned", "textIsEdgeAligned", "textIsEdgeAligned"),
]
# Enum-valued elements. The companion writes a name; the probe writes musxdom's ordinal, so the
# name has to be resolved back through musxdom's own mapping. An absent element is the default,
# which for all four is ordinal 0.
TEXT_ENUMS = [
    ("date_format", "dateFormat", "dateFormat",
     {"short": 0, "long": 1, "abbrev": 2}),
    ("text_justify", "textJustify", "textJustify",
     {"left": 0, "center": 1, "right": 2, "full": 3, "forcedFull": 4}),
    # HorizontalAlignment is AlignJustify: left, right, center -- not the TextJustify order.
    ("text_horz_align", "textHorzAlign", "textHorzAlign",
     {"left": 0, "right": 1, "center": 2}),
    ("text_vert_align", "textVertAlign", "textVertAlign",
     {"top": 0, "center": 1, "bottom": 2}),
]
# Symbol-insert fields: probe key, companion element, reported member.
INSERT_NUMERIC = [
    ("tracking_before", "trackingBefore", "trackingBefore"),
    ("tracking_after", "trackingAfter", "trackingAfter"),
    ("baseline_shift_perc", "baselineShiftPerc", "baselineShiftPerc"),
    ("sym_char", "symChar", "symChar"),
]
INSERT_TYPES = ["sharp", "flat", "natural", "dblSharp", "dblFlat"]


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


def text_coverage(obs):
    text = obs.get("text_options")
    if not text:
        return None
    scalars = [v for k, v in text.items() if k.startswith("origin_")]
    insert_origins = []
    for ins in text.get("inserts", []):
        insert_origins.extend(v for k, v in ins.items() if k.startswith("origin_"))
    return {
        "scalar_recovered": recovered_origins(scalars),
        "scalar_total": len(scalars),
        "insert_recovered": recovered_origins(insert_origins),
        "insert_total": len(insert_origins),
        "dangling_fonts": sum(1 for i in text.get("inserts", []) if i.get("dangling_font")),
        "any_recovered": recovered_origins(scalars) + recovered_origins(insert_origins) > 0,
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
    """Mirror musxdom's normalizeFontName. Comparison only; spellings are kept.

    musxdom strips whitespace and lowercases ASCII, and keeps everything else. Stripping
    all punctuation instead is not the same rule and disagrees with the probe's own
    normalized_font_name on any face whose name carries punctuation -- Finale 27 writes an
    unresolvable comparator as "Missing Font (110)", which the two rules normalized to
    "missingfont110" and "missingfont(110)", inventing 16 differences that were only the
    two normalizers disagreeing with each other.
    """
    return "".join(c.lower() if c.isascii() else c
                   for c in (name or "") if not (c.isascii() and c.isspace()))


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
    lyric = {}
    lo = re.search(r"<lyricOptions>(.*?)</lyricOptions>", xml, re.S)
    if lo:
        body = lo.group(1)
        for probe_key, element in LYRIC_NUMERIC:
            m = re.search(r"<%s>(-?\d+)</%s>" % (element, element), body)
            lyric[probe_key] = int(m.group(1)) if m else 0
        for probe_key, element in LYRIC_BOOLEAN:
            lyric[probe_key] = ("<%s/>" % element) in body
        for probe_key, element, mapping in LYRIC_ENUMS:
            m = re.search(r"<%s>(\w+)</%s>" % (element, element), body)
            lyric[probe_key] = mapping.get(m.group(1), 0) if m else 0
        for probe_key, element in LYRIC_TEXT:
            m = re.search(r"<%s>(.*?)</%s>" % (element, element), body, re.S)
            lyric[probe_key] = unescape_xml(m.group(1)) if m else ""
        syllables = {}
        for m in re.finditer(
                r'<lyricSyllPosStyle type="(\w+)">(.*?)</lyricSyllPosStyle>', body, re.S):
            inner = m.group(2)
            align = re.search(r"<align>(\w+)</align>", inner)
            justify = re.search(r"<justify>(\w+)</justify>", inner)
            syllables[m.group(1)] = {
                "align": ALIGN_JUSTIFY.get(align.group(1), 0) if align else 0,
                "justify": ALIGN_JUSTIFY.get(justify.group(1), 0) if justify else 0,
                "on": "<on/>" in inner,
            }
        lyric["syllables"] = syllables
        connects = {}
        for m in re.finditer(
                r'<wordExtConnectStyle type="(\w+)">(.*?)</wordExtConnectStyle>', body, re.S):
            inner = m.group(2)
            index = re.search(r"<connectIndex>(\w+)</connectIndex>", inner)
            x = re.search(r"<xOffset>(-?\d+)</xOffset>", inner)
            y = re.search(r"<yOffset>(-?\d+)</yOffset>", inner)
            connects[m.group(1)] = {
                "connect_index": WORD_EXT_CONNECT_INDEX.get(index.group(1), 0) if index else 0,
                "x": int(x.group(1)) if x else 0,
                "y": int(y.group(1)) if y else 0,
            }
        lyric["connects"] = connects

    stem = {}
    so = re.search(r"<stemOptions>(.*?)</stemOptions>", xml, re.S)
    if so:
        body = so.group(1)
        for probe_key, element in STEM_NUMERIC:
            m = re.search(r"<%s>(-?\d+)</%s>" % (element, element), body)
            stem[probe_key] = int(m.group(1)) if m else 0
        for probe_key, element in STEM_BOOLEAN:
            stem[probe_key] = ("<%s/>" % element) in body
        connections = []
        for m in re.finditer(r'<stemConnect index="(\d+)">(.*?)</stemConnect>', body, re.S):
            entry = {}
            for probe_key, element in STEM_CONNECT_FIELDS:
                mm = re.search(r"<%s>(-?\d+)</%s>" % (element, element), m.group(2))
                entry[probe_key] = int(mm.group(1)) if mm else 0
            fid = re.search(r"<font>(\d+)</font>", m.group(2))
            entry["font_name"] = normalize_font(
                names.get(fid.group(1) if fid else "0", ""))
            connections.append(entry)
        stem["connections"] = connections

    spacing = {}
    mso = re.search(r"<musicSpacingOptions>(.*?)</musicSpacingOptions>", xml, re.S)
    if mso:
        for probe_key, element in SPACING_NUMERIC:
            m = re.search(r"<%s>(-?\d+)</%s>" % (element, element), mso.group(1))
            spacing[probe_key] = int(m.group(1)) if m else 0

    layers = {}
    for m in re.finditer(r'<layerAtts cmper="(\d+)">(.*?)</layerAtts>', xml, re.S):
        off = re.search(r"<restOffset>(-?\d+)</restOffset>", m.group(2))
        layers[int(m.group(1))] = int(off.group(1)) if off else 0

    graphics = {}
    for probe_key, element in [("page_graphic_assigns", "pageGraphicAssign"),
                               ("shape_graphic_assigns", "shapeGraphicAssign")]:
        found = {}
        for m in re.finditer(
                r'<%s cmper="(\d+)" inci="(\d+)">(.*?)</%s>' % (element, element), xml, re.S):
            entry = {}
            for pk, el in GRAPHIC_FIELDS:
                mm = re.search(r"<%s>(-?\d+)</%s>" % (el, el), m.group(3))
                entry[pk] = int(mm.group(1)) if mm else 0
            found[(int(m.group(1)), int(m.group(2)))] = entry
        graphics[probe_key] = found

    meas_graphics = {}
    for m in re.finditer(
            r'<measGraphicAssign cmper1="(\d+)" cmper2="(\d+)" inci="(\d+)">(.*?)'
            r'</measGraphicAssign>', xml, re.S):
        entry = {}
        for pk, el in MEAS_GRAPHIC_FIELDS:
            mm = re.search(r"<%s>(-?\d+)</%s>" % (el, el), m.group(4))
            entry[pk] = int(mm.group(1)) if mm else 0
        meas_graphics[(int(m.group(1)), int(m.group(2)), int(m.group(3)))] = entry

    shapes = {}
    for m in re.finditer(r'<shapeDef cmper="(\d+)"[^>]*>(.*?)</shapeDef>', xml, re.S):
        inst = re.search(r"<instList>(\d+)</instList>", m.group(2))
        shapes[int(m.group(1))] = int(inst.group(1)) if inst else 0

    ss_lines = {}
    for m in re.finditer(r'<ssLineStyle cmper="(\d+)"[^>]*>(.*?)</ssLineStyle>', xml, re.S):
        body = m.group(2)

        def num(element, where=body):
            mm = re.search(r"<%s>(-?\d+)</%s>" % (element, element), where)
            return int(mm.group(1)) if mm else 0

        def block(name):
            mm = re.search(r"<%s>(.*?)</%s>" % (name, name), body, re.S)
            return mm.group(1) if mm else ""

        def named(element, table, absent, where=body):
            """An enumerated element, as the int musxdom declares it. Absent means the default."""
            mm = re.search(r"<%s>(\w+)</%s>" % (element, element), where)
            return table[mm.group(1) if mm else absent]

        char_params, solid_params, dashed_params = (
            block("charParams"), block("solidParams"), block("dashedParams"))
        ss_lines[int(m.group(1))] = {
            "line_style": named("lineStyle", SS_LINE_STYLES, "char"),
            "cap_start_type": named("lineCapStartType", SS_LINE_CAPS, "none"),
            "cap_end_type": named("lineCapEndType", SS_LINE_CAPS, "none"),
            "cap_start_arrow_id": num("lineCapStartArrowID"),
            "cap_end_arrow_id": num("lineCapEndArrowID"),
            "cap_start_hook_length": num("lineCapStartHookLength"),
            "cap_end_hook_length": num("lineCapEndHookLength"),
            "left_start_x": num("leftStartX"),
            "left_start_y": num("leftStartY"),
            "line_start_x": num("lineStartX"),
            "line_end_x": num("lineEndX"),
            "line_cont_x": num("lineContX"),
            "line_char": num("lineChar", char_params),
            "char_font_id": num("fontID", char_params),
            "char_font_size": num("fontSize", char_params),
            "solid_width": num("lineWidth", solid_params),
            "dash_on": num("dashOn", dashed_params),
            "dash_off": num("dashOff", dashed_params),
        }

    texts = {}
    for probe_key, element, _label in TEXT_CLASSES:
        found = {}
        for m in re.finditer(
                r'<%s number="(\d+)"[^>]*>(.*?)</%s>' % (element, element), xml, re.S):
            found[int(m.group(1))] = m.group(2)
        texts[probe_key] = found
    file_info = {}
    for m in re.finditer(r'<fileInfo type="(\w+)">(.*?)</fileInfo>', xml, re.S):
        file_info[m.group(1)] = m.group(2)

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
    text = {}
    inserts = {}
    to = re.search(r"<textOptions>(.*?)</textOptions>", xml, re.S)
    if to:
        body = to.group(1)
        # Strip the insert elements before reading the scalars: <symChar> and the tracking
        # elements live inside them and would otherwise be picked up as document scalars.
        outer = re.sub(r"<insertSymbolInfo\b.*?</insertSymbolInfo>", "", body, flags=re.S)
        for _, tag, _ in TEXT_NUMERIC:
            m = re.search(r"<%s>(-?\d+)</%s>" % (tag, tag), outer)
            text[tag] = int(m.group(1)) if m else 0
        for _, tag, _ in TEXT_BOOLEAN:
            text[tag] = bool(re.search(r"<%s/>|<%s>1</%s>" % (tag, tag, tag), outer))
        for _, tag, _, mapping in TEXT_ENUMS:
            m = re.search(r"<%s>(\w+)</%s>" % (tag, tag), outer)
            text[tag] = mapping.get(m.group(1), -1) if m else 0
        # Line spacing: whichever spelling the companion carries. Absent means absent, which
        # is a real difference from a reader that supplied one, so no default is invented.
        for tag in ("textLineSpacingPercent", "textLineSpacingEvpu"):
            m = re.search(r"<%s>(-?\d+)</%s>" % (tag, tag), outer)
            text[tag] = int(m.group(1)) if m else None
        for m in re.finditer(
                r'<insertSymbolInfo type="(\w+)">(.*?)</insertSymbolInfo>', body, re.S):
            d = m.group(2)
            entry = {}
            for _, tag, _ in INSERT_NUMERIC:
                mm = re.search(r"<%s>(-?\d+)</%s>" % (tag, tag), d)
                entry[tag] = int(mm.group(1)) if mm else 0
            sf = re.search(r"<symFont>(.*?)</symFont>", d, re.S)
            fid = re.search(r"<fontID>(\d+)</fontID>", sf.group(1)) if sf else None
            size = re.search(r"<fontSize>(-?\d+)</fontSize>", sf.group(1)) if sf else None
            entry["font_name"] = normalize_font(
                names.get(fid.group(1) if fid else "0", ""))
            entry["font_size"] = int(size.group(1)) if size else 0
            inserts[m.group(1)] = entry
    return {"fonts": fonts,
            "font_names": {normalize_font(v) for v in names.values() if v},
            "clefs": clefs, "clef_scalars": scalars, "mmrest": mmrest,
            "text": text, "text_inserts": inserts,
            "texts": texts, "file_info": file_info,
            "lyric": lyric, "stem": stem, "spacing": spacing, "layers": layers,
            "graphics": graphics, "shapes": shapes, "ss_lines": ss_lines,
            "meas_graphics": meas_graphics}


def compare_texts(obs, comp, out):
    """Compare every recovered text record with the companion's own.

    Three outcomes are counted separately from a plain match, because each is a difference the
    reader makes on purpose and none of them means a record was read wrongly:

      font-spelling-only  the texts agree once the font command is normalized away
      reader-only         a record the companion does not carry
      companion-only      a record the reader does not produce, most often the part name
                          Finale 27 synthesizes, or a style prefix it supplies where the source
                          states none
    """
    for probe_key, _element, label in TEXT_CLASSES:
        ours = {item["number"]: item["text"] for item in (obs.get(probe_key) or [])}
        theirs = comp["texts"].get(probe_key, {})
        for number in sorted(set(ours) | set(theirs)):
            a, b = ours.get(number), theirs.get(number)
            if a is None:
                out[(label + ".record", "text", "companion-only")] += 1
            elif b is None:
                out[(label + ".record", "text", "reader-only")] += 1
            elif a == b:
                out[(label + ".record", "text", "match")] += 1
            elif normalize_text(a) == normalize_text(b):
                out[(label + ".record", "text", "font-spelling-only")] += 1
            else:
                out[(label + ".record", "text", "differ")] += 1

    # File Info is keyed by type name in the companion and by musxdom's ordinal here, so it is
    # compared through that enumeration rather than by number.
    file_info_types = ["title", "composer", "copyright", "description",
                       "lyricist", "arranger", "subtitle"]
    ours = {item["number"]: item["text"] for item in (obs.get("file_info_texts") or [])}
    for ordinal, type_name in enumerate(file_info_types, start=1):
        a, b = ours.get(ordinal), comp["file_info"].get(type_name)
        if a is None and b is None:
            continue
        if a is None:
            out[("FileInfoText." + type_name, "text", "companion-only")] += 1
        elif b is None:
            out[("FileInfoText." + type_name, "text", "reader-only")] += 1
        else:
            out[("FileInfoText." + type_name, "text", "match" if a == b else "differ")] += 1


def compare_lyric_options(obs, comp, out):
    ours = obs.get("lyric_options")
    theirs = comp.get("lyric")
    if not ours or not theirs:
        return
    fields = ([(k, e) for k, e in LYRIC_NUMERIC] + [(k, e) for k, e in LYRIC_BOOLEAN]
              + [(k, e) for k, e, _m in LYRIC_ENUMS] + [(k, e) for k, e in LYRIC_TEXT])
    for field, _element in fields:
        a, b = ours.get(field), theirs.get(field)
        if a is None or b is None:
            continue
        origin = ours.get("origin_" + to_member(field), "absent")
        # A companion omits an element whose value is musxdom's own default, and musxdom fills
        # that default in during construction, so the reader holds it while the companion shows
        # nothing. Where the reader did not take the value from the source the two agree about
        # the default and the silence means the same thing; only a source-derived value can
        # disagree with an omission.
        if b in ("", 0, False) and origin != "legacy-mus" and a != b:
            out[("LyricOptions." + field, origin, "default-omitted-by-companion")] += 1
            continue
        out[("LyricOptions." + field, origin,
             "preserved" if a == b else "differs")] += 1

    for name, style in (ours.get("syllable_pos_styles") or {}).items():
        other = (theirs.get("syllables") or {}).get(name)
        if other is None:
            out[("LyricOptions.syllablePosStyles", "absent", "companion-only")] += 1
            continue
        for member in ("align", "justify", "on"):
            out[("LyricOptions.syllablePosStyles." + member, "legacy-mus",
                 "preserved" if style.get(member) == other.get(member) else "differs")] += 1

    for name, style in (ours.get("word_ext_connect_styles") or {}).items():
        other = (theirs.get("connects") or {}).get(name)
        if other is None:
            out[("LyricOptions.wordExtConnectStyles", "absent", "companion-only")] += 1
            continue
        for member in ("connect_index", "x", "y"):
            out[("LyricOptions.wordExtConnectStyles." + member, "legacy-mus",
                 "preserved" if style.get(member) == other.get(member) else "differs")] += 1


def compare_simple_class(obs, comp, out, probe_key, comp_key, label, numeric, boolean=()):
    """Compare a flat scalar class field by field."""
    ours = obs.get(probe_key)
    theirs = comp.get(comp_key)
    if not ours or not theirs:
        return
    for field, _element in list(numeric) + list(boolean):
        a, b = ours.get(field), theirs.get(field)
        if a is None or b is None:
            continue
        origin = ours.get("origin_" + to_member(field), "absent")
        out[(label + "." + field, origin, "preserved" if a == b else "differs")] += 1


def compare_stem_connections(obs, comp, out):
    ours = (obs.get("stem_options") or {}).get("stem_connections") or []
    theirs = (comp.get("stem") or {}).get("connections") or []
    for index in range(max(len(ours), len(theirs))):
        if index >= len(ours):
            # The reader stops at the first element with no symbol; Finale 27 carries the
            # trailing elements through. The difference is intended, so it is named rather
            # than counted as a plain absence.
            out[("StemOptions.connection", "absent", "past-terminator")] += 1
            continue
        if index >= len(theirs):
            out[("StemOptions.connection", "absent", "reader-only")] += 1
            continue
        a, b = ours[index], theirs[index]
        for field, _element in STEM_CONNECT_FIELDS:
            if field == "font_id":
                # The comparator, because the companion writes it directly under <font> and it
                # is what the reader claims to have read. The face is reported beside it rather
                # than instead of it: a companion resolves the same comparator through its own
                # font table, which Finale 27 renumbers, so a face difference under a matching
                # comparator says something about the upgrade and not about the reader.
                if a.get(field) != b.get(field):
                    outcome = "differs"
                elif normalize_font(a.get("font_name") or "") == (b.get("font_name") or ""):
                    outcome = "preserved"
                else:
                    outcome = "renumbered-face"
            else:
                outcome = "preserved" if a.get(field) == b.get(field) else "differs"
            out[("StemOptions.connection." + field,
                 a.get("origin_" + to_member(field), "absent"), outcome)] += 1


def compare_layers(obs, comp, out):
    ours = {layer["cmper"]: layer for layer in (obs.get("layer_atts") or [])}
    theirs = comp.get("layers") or {}
    for cmper in sorted(set(ours) | set(theirs)):
        a, b = ours.get(cmper), theirs.get(cmper)
        if a is None:
            out[("LayerAttributes.restOffset", "absent", "companion-only")] += 1
        elif b is None:
            out[("LayerAttributes.restOffset", "absent", "reader-only")] += 1
        else:
            out[("LayerAttributes.restOffset", a.get("origin_restOffset", "absent"),
                 "preserved" if a["rest_offset"] == b else "differs")] += 1


def compare_graphics(obs, comp, out):
    for probe_key, label in [("page_graphic_assigns", "PageGraphicAssign"),
                             ("shape_graphic_assigns", "ShapeGraphicAssign")]:
        ours = {(g["cmper"], g["inci"]): g for g in (obs.get(probe_key) or [])}
        theirs = (comp.get("graphics") or {}).get(probe_key, {})
        for key in sorted(set(ours) | set(theirs)):
            a, b = ours.get(key), theirs.get(key)
            if a is None:
                out[(label + ".record", "absent", "companion-only")] += 1
                continue
            if b is None:
                out[(label + ".record", "absent", "reader-only")] += 1
                continue
            for field, _element in GRAPHIC_FIELDS:
                out[(label + "." + field, "legacy-mus",
                     "preserved" if a.get(field) == b.get(field) else "differs")] += 1


def compare_measure_graphics(obs, comp, out):
    ours = {(g["cmper1"], g["cmper2"], g["inci"]): g
            for g in (obs.get("meas_graphic_assigns") or [])}
    theirs = comp.get("meas_graphics") or {}
    for key in sorted(set(ours) | set(theirs)):
        a, b = ours.get(key), theirs.get(key)
        if a is None:
            out[("MeasureGraphicAssign.record", "absent", "companion-only")] += 1
            continue
        if b is None:
            out[("MeasureGraphicAssign.record", "absent", "reader-only")] += 1
            continue
        for field, _element in MEAS_GRAPHIC_FIELDS:
            out[("MeasureGraphicAssign." + field, "legacy-mus",
                 "preserved" if a.get(field) == b.get(field) else "differs")] += 1


# musxdom's declaration order for the two enums, which is what the probe writes as an int.
SS_LINE_STYLES = {"char": 0, "solid": 1, "dashed": 2}
SS_LINE_CAPS = {"none": 0, "hook": 1, "arrowheadPreset": 2, "arrowheadCustom": 3}

# Every field the probe emits for a custom line style, compared as plain numbers.
SS_LINE_FIELDS = (
    "line_style", "cap_start_type", "cap_end_type", "cap_start_arrow_id", "cap_end_arrow_id",
    "cap_start_hook_length", "cap_end_hook_length", "left_start_x", "left_start_y",
    "line_start_x", "line_end_x", "line_cont_x", "line_char", "char_font_id",
    "char_font_size", "solid_width", "dash_on", "dash_off")


def compare_ss_lines(obs, comp, out):
    """Custom line styles, keyed by comparator on each side independently."""
    ours = {line["cmper"]: line for line in (obs.get("ss_line_styles") or [])}
    theirs = comp.get("ss_lines") or {}
    for cmper in sorted(set(ours) | set(theirs)):
        a, b = ours.get(cmper), theirs.get(cmper)
        if a is None:
            # Finale's upgrade adds a built-in default the source never stored. Counted, so a
            # real absence is still visible, but expected rather than a reader gap.
            out[("SmartShapeCustomLine.record", "absent", "companion-only")] += 1
            continue
        if b is None:
            out[("SmartShapeCustomLine.record", "absent", "reader-only")] += 1
            continue
        for field in SS_LINE_FIELDS:
            out[("SmartShapeCustomLine." + to_member(field), "legacy-mus",
                 "preserved" if a.get(field) == b.get(field) else "differs")] += 1


def compare_shapes(obs, comp, out):
    ours = {sh["cmper"]: sh for sh in (obs.get("shape_defs") or [])}
    theirs = comp.get("shapes") or {}
    for cmper in sorted(set(ours) | set(theirs)):
        a, b = ours.get(cmper), theirs.get(cmper)
        if a is None:
            out[("ShapeDef.instList", "absent", "companion-only")] += 1
        elif b is None:
            out[("ShapeDef.instList", "absent", "reader-only")] += 1
        else:
            out[("ShapeDef.instList", "legacy-mus",
                 "preserved" if a["instruction_list"] == b else "differs")] += 1


def to_member(probe_key):
    """snake_case probe key -> the musxdom member name the origin is reported under."""
    head, *rest = probe_key.split("_")
    return head + "".join(part.capitalize() for part in rest)


def compare_companion(obs, comp):
    """Classify each comparable value. Returns Counter of (class, field, outcome)."""
    out = collections.Counter()
    compare_texts(obs, comp, out)
    compare_simple_class(obs, comp, out, "stem_options", "stem", "StemOptions",
                         STEM_NUMERIC, STEM_BOOLEAN)
    compare_stem_connections(obs, comp, out)
    compare_simple_class(obs, comp, out, "spacing_options", "spacing", "MusicSpacingOptions",
                         SPACING_NUMERIC)
    compare_layers(obs, comp, out)
    compare_graphics(obs, comp, out)
    compare_shapes(obs, comp, out)
    compare_ss_lines(obs, comp, out)
    compare_lyric_options(obs, comp, out)
    compare_measure_graphics(obs, comp, out)
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

    text = obs.get("text_options")
    if text and comp["text"]:
        for key, tag, member in TEXT_NUMERIC + TEXT_BOOLEAN:
            outcome = "preserved" if text[key] == comp["text"][tag] else "differs"
            out[("TextOptions." + tag, text.get("origin_" + member, "absent"), outcome)] += 1
        for key, tag, member, _ in TEXT_ENUMS:
            outcome = "preserved" if text[key] == comp["text"][tag] else "differs"
            out[("TextOptions." + tag, text.get("origin_" + member, "absent"), outcome)] += 1
        # Line spacing is one value in two spellings. Compare the pair, so a reader that put
        # the right number under the wrong spelling is a difference rather than a match.
        ours_spacing = (text["line_spacing_percent"], text["line_spacing_evpu"])
        theirs_spacing = (comp["text"]["textLineSpacingPercent"],
                          comp["text"]["textLineSpacingEvpu"])
        spacing_origin = text.get("origin_textLineSpacingPercent", "absent")
        if spacing_origin == "absent":
            spacing_origin = text.get("origin_textLineSpacingEvpu", "absent")
        out[("TextOptions.lineSpacing", spacing_origin,
             "preserved" if ours_spacing == theirs_spacing else "differs")] += 1

        ours_inserts = {i["type"]: i for i in text.get("inserts", [])}
        for name in INSERT_TYPES:
            ours = ours_inserts.get(name)
            theirs = comp["text_inserts"].get(name)
            if ours is None or not ours.get("present") or theirs is None:
                out[("TextOptions.symbolInserts", "pool",
                     "absent-in-companion" if theirs is None else "absent-in-reader")] += 1
                continue
            for key, tag, member in INSERT_NUMERIC:
                outcome = "preserved" if ours[key] == theirs[tag] else "differs"
                out[("TextOptions.symbolInserts." + tag,
                     ours.get("origin_" + member, "absent"), outcome)] += 1
            # The comparator is renumbered between pools, so only the resolved face compares.
            same_face = (ours.get("normalized_font_name")
                         or normalize_font(ours.get("font_name"))) == (theirs["font_name"] or "")
            out[("TextOptions.symbolInserts.symFont.face",
                 ours.get("origin_symFont.fontId", "absent"),
                 "preserved" if same_face else "differs")] += 1
            out[("TextOptions.symbolInserts.symFont.size",
                 ours.get("origin_symFont.fontSize", "absent"),
                 "preserved" if ours["font_size"] == theirs["font_size"] else "differs")] += 1

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
        t = text_coverage(obs)
        if t:
            e["tx_docs"] += 1
            e["tx_any"] += 1 if t["any_recovered"] else 0
            e["tx_scalar_recovered"] += t["scalar_recovered"]
            e["tx_scalar_total"] += t["scalar_total"]
            e["tx_insert_recovered"] += t["insert_recovered"]
            e["tx_insert_total"] += t["insert_total"]
            e["tx_dangling_fonts"] += t["dangling_fonts"]
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
              f"{'TextOptions':>20}  {'FontDefinition':>20}")
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
        tx = (f"{e['tx_any']}/{e['tx_docs']} docs "
              f"{e['tx_scalar_recovered'] + e['tx_insert_recovered']}f")
        fd = f"{e['fd_any']}/{e['fd_docs']} docs {e['fd_recovered']}f"
        print(f"{epoch:<14}{e['docs']:>7}{e.get('occurrences', 0):>8}  "
              f"{clef:>22}  {fo:>20}  {mm:>20}  {tx:>20}  {fd:>20}")
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
                           ("tx_any", "TextOptions"),
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
        print(f"  {'field':<52}{'origin':<18}{'outcome':<24}{'count':>8}")
        for (field, origin, outcome), n in sorted(
                outcomes.items(), key=lambda kv: (-kv[1], kv[0])):
            print(f"  {field:<52}{origin:<18}{outcome:<24}{n:>8}")

    print()
    print("RESULT:", "ACCEPTED" if ok else "NOT ACCEPTED")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
