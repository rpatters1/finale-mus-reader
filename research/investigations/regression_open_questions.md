# Open questions from the regression comparison

**Covers:** The stem-connection font provenance and the three wrong readings of the comparison instrument that preceded it.
**Read when:** Interpreting a companion-comparison disagreement, or before trusting a comparison script's field lookup.
**Confidence:** confirmed for stem connection fonts across three corpora.

## Open questions the regression comparison surfaced

Two differences against companions are unexplained and are recorded here rather than suppressed
in the comparison, because a regression that hides what it cannot explain is worth less than one
that counts it.

**Stem connection fonts are confirmed on every connection in every survey**, comparator and
resolved face alike: 53,125 preserved, none differing. The field's provenance is worth recording
because it was the weakest in the element. It is not in the distilled framework mapping, which
has no row for this table at all, and the notes above confirm the element's other properties —
the Evpu/Efix adjustments, the symbol's zero high byte, the terminator rule — without ever
confirming word 0. It rests on musxdom's own `StemConnection` field order matched positionally
to the six stored words. That correspondence is now corroborated across three corpora.

Getting there took three wrong readings, all of the instrument rather than the format, and they
are recorded because each is a trap a later comparison can fall into again:

  * musxdom maps `StemConnection::fontId` to the node `<font>`, not `<fontID>`. Reading the
    latter found nothing, defaulted to comparator zero and so resolved every companion
    connection to the default music font. The disagreement then looked like a systematic
    percussion-versus-base pattern affecting exactly twenty connections per document, which is
    a far more convincing shape than random noise.
  * Repairing it by string replacement changed the *first* `<fontID>` in the comparison script,
    which belongs to `FontOptions`, leaving the stem one untouched. That silently moved
    `FontOptions` face disagreements from 1,984 to 91,710 while appearing to fix stem fonts.
  * An intermediate reading concluded the companion's font table had been renumbered. It had
    not: the companion resolves comparator 5 to `Maestro Percussion` exactly as the source does.
