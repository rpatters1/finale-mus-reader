# Chord options

**Covers:** Legacy recovery boundaries for `musx::dom::options::ChordOptions`.
**Read when:** Changing `src/import/options/chord_options.cpp` or interpreting its coverage differences.
**Confidence:** strong for the accidental-lift boundary; other mappings remain source-derived.

Selector 37 words 3--5 store `chordSharpLift`, `chordFlatLift`, and `chordNaturalLift` from
Finale 3.7 onward. Before Finale 3.7, the importer supplies 12 for all three as
`LegacyBehavior`. This is a version gate within the uncompressed epoch: the selector exists in
earlier documents, but its earlier shape does not carry all three independently.

The Finale 3.7 addendum identifies that release as a significant expansion of chord features.
The controlled Finale 3.7.2 baseline stores 12 in all three locations, and the Finale 97 source
`mus-1e8f7cc5864a248e` stores 24 in each; its companion preserves all three values. Controlled
Finale 1.0.0 and 2.6.3 baselines instead store 12 only at word 3 and zero at words 4 and 5.
