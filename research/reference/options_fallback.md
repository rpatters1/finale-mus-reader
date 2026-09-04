# Options fallback strategy

**Covers:** The sequence that seeds a structurally complete options pool from the pinned Finale 27 baseline, and the three `ValueOrigin` values that report where each value came from.
**Read when:** Adding an option overlay, or deciding whether a value should be asserted in code at all.
**Confidence:** project rule; the baseline resources are fixed and hashed.

musxdom expects a structurally complete options pool. Implement fallback options
with this sequence:

1. Select the pinned Finale 27 macOS or Windows baseline explicitly, preferring
   a source-platform match when reliable.
2. Inflate the embedded gzip bytes with zlib and parse the raw EnigmaXML with
   the configured musxdom XML backend.
3. Seed or clone only the complete options pool into the imported document.
4. Overlay every confidently recovered legacy option value.
5. Leave absent, unknown, or unsupported values at the Finale 27 default. Prefer this even
   where the source era's behavior is known, whenever the baseline already carries the value
   that behavior implies. The baseline is generated from committed resources whose hashes
   this document records, so it is effectively as fixed as a constant, and a value asserted
   in code beside a baseline that already agrees is a second copy of the same fact.
6. Report recovered values separately from synthesized defaults, and separately again from
   values determined by how the source version behaved when it had no option to store them.
   `ValueOrigin` names the three: `LegacyMus`, `LegacyBehavior`, and `Finale27Default`.
   Reserve `LegacyBehavior` for a value the baseline does **not** already supply, or supplies
   wrongly — an era that always did something later versions let you turn off, where reading
   the later location would assert the opposite. Where a capture pass establishes such a field
   before the mapping tables run, the tables leave its report entry alone rather than overwriting
   it with a default.

## Distinguishing "not mapped" from "mapped and defaulted"

`Finale27Default` means the field is a known member of the recovery model, but the applicable
source layout does not supply it and no legacy behavior overrides the baseline. Two further cases
must not collapse into it:

- **`Unmapped`** — the field could have a legacy source, but none has been located in any layout
  or epoch. This says nothing about how hard anyone looked: it covers a field nobody has
  investigated and a field whose known records provably do not carry it, since both leave a value
  no evidence supports and both may still be recovered from a record the reader has not reached.
  Which of the two a field is belongs in its class's notes under `research/format/`, where the
  evidence can be stated, and a class is not partial merely because a field reports this.
- **`MusxOnly`** — evidence establishes that the field postdates every supported legacy layout
  and therefore cannot be recovered.

In either case the value remains default-initialized for a source-owned object and stays at the
seeded Finale 27 value for an options object. The distinction is what keeps an importer that maps
a field in one epoch and deliberately falls back in another from looking like an importer that
never investigated the field at all.

Where existing instrumentation necessarily labels every untouched seeded value
`Finale27Default`, preserve that behavior only when the probe also emits an explicit structured
mapping status separating `unmapped` from `mapped-but-defaulted`. The probe is the authority for
that status; the Python report must not infer it from values, epochs, incidental origins, or the
presence of a mapping table. Prefer expressing `Unmapped` and `MusxOnly` directly in `ValueOrigin`
where that cleanly avoids a parallel status model.

Never leak fallback measures, staves, entries, text, document identity, header
values, or other score content into an imported document. The fallback document
must not remain the owner of options placed in the imported document.
