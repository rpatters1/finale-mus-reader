# Options-pool external-cmper TODO

Audit of musxdom `Options.h`: fields in the options pool that refer to objects
outside that pool and are not yet recovered by this reader.

Keep this list terse and actionable. Record completed items only in the short
section below; do not expand the TODO bullets into investigation notes.

- `ChordOptions::fretStyleId`, `fretInstId` -> `details::FretboardStyle`,
  `details::FretInstrument`. Recover and resolve the two default fretboard
  references.
- `LyricOptions::altHyphenFont` -> `others::FontDefinition`. Find its legacy
  location, recover the font tuple, and resolve its definition.
- `MultimeasureRestOptions::shapeDef` -> `others::ShapeDef`. Recover and
  resolve the H-bar shape.
- `RepeatOptions::showOnStaffListNumber` -> `others::StaffListRepeatName`,
  `StaffListRepeatScore`, `StaffListRepeatParts`, and their override classes.
  Recover the linked repeat staff-list family.
- `SmartShapeOptions::ssLineStyleCmp*` -> `others::SmartShapeCustomLine`
  (four fields: custom, glissando, tab slide, tab bend curve). Recover the
  referenced line styles; include their `CharParams::font` references.
- `TextOptions::symbolInserts[*].symFont` -> `others::FontDefinition`. Find
  the legacy location, recover the inserted-symbol font tuples, and resolve
  their definitions.


## Completed

- `FontDefinition`
- default `FontOptions` font IDs
- `ClefOptions::ClefDef::font`
- `ClefOptions::ClefDef::shapeId` — clefs 17 and 18 import their source-owned
  shapes on demand.
- `StemOptions::StemConnection::fontId`
