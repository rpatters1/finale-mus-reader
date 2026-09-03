# LayerAttributes investigations

**Covers:** How the `LA` layout, its flag mapping, the behavior of a layer with no record, and the pre-Finale-2002 playback and spacing rule were established.
**Read when:** Proposing a hypothesis about layer attributes, or before re-opening any of the questions below.

## 2026-09-03 — Locating the record and its flag word

**Question.** The class existed as a prototype stub mapping one field, `restOffset`, from tag `LA`
word 0. Where do the other ten musxdom members live, and in which epochs?

**Method.** The public Framework layer-preferences class
([source](https://pdk.finalelua.com/ff__prefs_8h_source.html), accessed 2026-09-03) names one flag
constant per musxdom member through its accessors, which establishes the member-to-flag
correspondence. It does not carry the constants' values, the record layout, or the tag; the
online 1997 `edata.h` at GUIDOLib commit `9f74ba9b3e287f240bbd454c2259fc3f7737c6ad` predates
layers entirely and mentions neither. Those three are `private-framework-derived`: an authorized
private Framework header supplies the tag as the two characters `LA` under the preferences record
type, a record of one rest-offset word followed by one 16-bit flag word, and the ten mask values
tabulated in [`../format/others/layer_attributes.md`](../format/others/layer_attributes.md).

That private description is two words long, but the ETF `^LA` rows and the decoded MUS rows both
carry six words with the flag in slot 5, not slot 1. The stored row therefore governs, and the
private description is the loaded view rather than the disk layout.
`tools/investigations/record_dump` over the tracked fixtures supplied the words directly.

**Result.** Layout and flag mapping as recorded in
[`../format/others/layer_attributes.md`](../format/others/layer_attributes.md). Two independent
checks agree on the mapping:

- The values decode as the familiar Finale defaults. `0x0f80` on layer 0 is freeze stems and ties,
  stems up, adjust floating rests by, apply only if notes in other layers, and flip ties, beside a
  rest offset of `6`; layer 1 is the same word without `freezeLayUp` and a rest offset of `-6`.
- The Finale 27 companions agree on every member of every layer from Finale 2002 onward, and
  disagree on exactly two members before it (below).

The zlib class id was resolved by elimination and then confirmed: `RECORD_CATALOG.md` ranked both
`0x008d` and `0x00a3` as `layerAtts` candidates on count correlation, but `0x008d` is the `FI`
family and carries payloads of 12, 36, and 48 bytes, while `0x00a3` carries four records at
comparators 0–3 with a 12-byte payload whose bytes are identical to the fixed rows of the same
document's DCL-era sibling.

## 2026-09-03 — Which releases store the row (**answered**)

**Question.** `LEGACY_OPTION_MAPPINGS.md` records 28 of 28 Finale 3.0-3.7 files in
`rpatters1-main` recovering all four layer offsets. No `LA` row occurred in any of the 24 Finale
3.7.2 fixtures in `tracked-evidence`, nor in the Finale 2000 re-save of one of them. Both could
not describe the same population.

**Result.** `tests/evidence/F372/F372-layer-adjrests.mus` settles it: the row is written on
demand. Saved from `F372-baseline` with rest offsets on all four layers and the checkbox enabled
on two, its only decoded change is four new `LA` rows -- word 0 the rest offset, word 5 `0x0200`
on the two enabled layers -- and its ETF repeats them. The private corpus holds authored
documents, whose layer settings have been touched; the tracked fixtures are purpose-built saves
left at the defaults. Neither measurement was wrong and the conflict is withdrawn.

**Refuted along the way.** The prediction that a pre-Finale-97 release kept layer state in some
*other* record. It keeps it in this one, and writes nothing when there is nothing to write. The
three candidate explanations recorded before the fixture arrived -- a boundary inside 3.0-3.7, a
measurement that counted seeded defaults as recoveries, and a shared property of the tracked
3.7.2 saves -- are all superseded by that single observation.

## 2026-09-03 — Playback and spacing before Finale 2002 (**answered**)

**Observation.** Across the tracked survey, every Finale 3.7.2, 97, 98, 2000, and 2001 document
that carries a row disagreed with its Finale 27 companion on exactly `playback` and
`affectSpacing`, on all four layers, and on nothing else: the source stores `false` and the
companion writes `true`. 256 leaves in all. Every Finale 2002 and later document agreed on all
eleven members.

The same private description marks the playback, spacing, hidden-notes, hide-when-inactive and
hidden-layers flags as added later than the rest, which matches the
flag-word values changing from `0x0f80`/`0x0b80`/`0x0000` to `0x7f80`/`0x7b80`/`0x3000` exactly at
Finale 2002.

**Direct check.** Finale 2000's layer dialog offers neither control, observed in the running
application on 2026-09-03. Finale 2001 could not be checked the same way -- that installation
would not run -- so the release immediately below the boundary is the one release whose UI has
not been seen. It stores the same flag words as Finale 97, 98 and 2000, and its companions write
both settings true, so nothing distinguishes it from the releases either side of it except the
missing observation.

**Result.** Neither setting exists before Finale 2002, so the era's behavior determines both and
the stored bit is meaningless. The reader supplies `true` below the boundary. The three other late
flags need no such treatment: their absent behavior is the `false` the cleared bit already
produces.

**Provenance.** Reported `Finale27Default`, not `LegacyBehavior`, because the pinned baseline
already holds `true` for both on all four layers, and the fallback rule reserves `LegacyBehavior`
for a value the baseline does not supply. The override is still real -- a stored `false` is
discarded -- but what replaces it is the baseline's own value, so the report says so.

**Gate.** A version gate, `sourcePredatesVersion(profile, DclLegacy, finale2002)`. The boundary is
inside the DCL epoch, and the record's shape is identical on both sides, so no structural marker
exists; the bit content would serve only as a heuristic over user-editable values.

## 2026-09-03 — What a layer with no record means (**answered**)

**Question.** If a release simply omits the row, what did the layer actually do -- and can the
reader say, rather than falling back to the Finale 27 new-document defaults?

**Method.** For all 221 tracked fixtures with a companion, detect the row in the source (tag `LA`
below the zlib epoch, class `0x00a3` within it) and decode the companion's own `<layerAtts>`
directly from `score.dat`. Group the 98 sources that store no row by their companion's values.

**Result.** One signature, 98 of 98, across Finale 1.0.0 (45), 2.6.3 (28), 3.7.2 (24) and one
Finale 2000 re-save: every layer has `restOffset` 0 with `playback` and `spacing` set and nothing
else. No variation between documents or between layers.

That alone would only establish what Finale 27 synthesizes on import. `F372-layer-adjrests`
separates synthesis from behavior: only "Adjust Floating Rests by" was enabled, and the rows
Finale 3.7.2 created carry only `0x0200`, with no freeze bits. The era's own defaults are
therefore all-clear, which is what the companions show, so this is `LegacyBehavior` and not an
upgrade artifact.

**Adopted.** The reader applies those values to any layer whose row is absent. Against the pinned
baseline, layers 2 and 3 agree on all eleven members and stay `Finale27Default`; layers 0 and 1
disagree on seven each and report `LegacyBehavior`. Over the tracked survey this removes 1,372 of the 1,620 `layer_atts` unexpected
differences; the pre-2002 rule above removes the remaining 248, and the class then agrees with
every companion in the cohort.

**Limit.** All 98 are big-endian macOS saves. The Coda-era Windows documents, which state no
version, live only in `rpatters1-installs` and are untested here; the rule keys on the row's
absence rather than on a version, so it does not depend on reading one.
