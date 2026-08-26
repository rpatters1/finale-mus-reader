# Options-pool external-cmper TODO

Audit of musxdom `Options.h`: fields in the options pool that refer to objects
outside that pool and are not yet recovered by this reader.

Keep this list terse and actionable. Record completed items only in the short
section below; do not expand the TODO bullets into investigation notes.

- `ChordOptions::fretStyleId`, `fretInstId` -> `details::FretboardStyle`,
  `details::FretInstrument`. Recover and resolve the two default fretboard
  references.
## Completed

- `FontDefinition`
- default `FontOptions` font IDs
- `ClefOptions::ClefDef::font`
- `ClefOptions::ClefDef::shapeId` — clefs 17 and 18 import their source-owned
  shapes on demand.
- `StemOptions::StemConnection::fontId`
- `SmartShapeOptions::ssLineStyleCmp*` — source-owned line definitions preserve
  their comparators. Before the class existed, glissando, tab slide, and tab
  bend curve copy the pinned baseline definitions in semantic order; the custom
  field remains unset. The musxdom importer resolves shape, font, and raw-text
  dependencies for copied definitions, all with `ShareMode::All`.
- `MultimeasureRestOptions::shapeDef` — the source's own comparator in every
  era; a reference the source does not define is kept as stored and noted at
  `Info`, which 319 zlib documents trigger and Finale treats as normal.
- `TextOptions::symbolInserts[*].symFont` — offset 10 of each element of
  `78(65534)` in all three insert layouts; Coda-banner has no such record and
  keeps the baseline tuple. Definitions resolve through
  `importFontDefinitionInto`.
- `LyricOptions::altHyphenFont` — closed with no legacy location, because none
  exists: the alternate hyphen font postdates Finale 2012. The pinned baseline
  states no `<altHyphenFont>` either, so nothing is imported and musxdom's
  `integrityCheck` supplies the object. A null member during the import is the
  signal that the baseline omitted the element.
- `RepeatOptions::showOnStaffListNumber` — closed with no legacy location. Controlled
  Finale 2005 and Finale 2012 saves create repeat staff-list objects but persist no
  selected-list reference; the pinned baseline supplies the document option. Repeat
  staff-list objects can be recovered later without reopening this options field.
