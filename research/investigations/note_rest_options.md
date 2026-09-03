# NoteRestOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-09-01 — Music-font-dependent rest-position defaults

- **Raw distinction:** `F100-baseline.mus` and `F100-music-font.mus` both omit selector `44` and
  store six zero words in selector `41`. The only changed numeric-global row is selector `02`,
  whose first three words change from `(0, 71, 0)` to `(12, 60, 0)` for the edited default music
  font. Neither source contains a `-48` word; their sole `-24` word is identical and belongs to
  selector `22`, not rest positioning.
- **Companion behavior:** The baseline companion explicitly stores 64th- and 128th-rest drops of
  `-24` and `-48`; the music-font companion omits both elements, which musxdom reads as zero. This
  is a change in Finale's chosen defaults during conversion, not recovery of a source value.
- **Approved classification:** A differing `NoteRestOptions` rest-drop leaf is
  `different_defaults` only when its source origin is `Finale27Default`. The tracked recapture
  completed all 191 sources and companions; this changes the class result from 8,593 equal plus
  two unexpected leaves to 8,593 equal plus two expected leaves, with no unexpected differences.
