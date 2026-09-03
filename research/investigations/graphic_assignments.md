# GraphicAssignments investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-26 — Page and measure graphic assignment layout

- **Hypothesis:** Page-attached and measure-attached legacy graphic assignments use the same
  record layout.
- **Structure:** Both assignments have the same 18-word semantic layout. An other row carries six
  payload words, so a page assignment fills three rows exactly. A detail row carries only five
  payload words after its second comparator, so a measure assignment requires four rows and the
  remaining two slots are zero filler. Zlib class `0x041d` preserves that padded 20-word stride.
  Word 8 is the packed horizontal alignment, vertical alignment, positioning reference, and
  preserve-aspect value in both families.
- **Controlled evidence:** The Finale 3.7.2 linked measure graphic, Finale 2006 embedded EPS, and
  Finale 2012 embedded GIF each carry word-8 bits whose decoded values match the independently
  parsed Finale 27 companion. Before the shared decoding was applied, the only five tracked
  companion differences were the nonzero `posFrom` and `fixedPerc` leaves those words predict;
  `hAlign` and `vAlign` already matched their zero-valued companion leaves by coincidence.
- **Display flags:** The supplied PDK declarations assign `0x0001/0x0002/0x0004/0x0008` to
  one/all/odd/even and `0x0010` to hidden in the assignment's display-flags word. Nine independently
  upgraded Finale 2012 page assignments store `0x0011` and preserve both `One` and `hidden=true`,
  distinguishing the packed interpretation from separate words 6 and 7.
- **Conclusion:** **Confirmed** for the common 18-word prefix across the uncompressed, DCL, and
  zlib epochs. The reader now uses one packed-position implementation for page, shape, and measure
  graphic assignments. A Coda-banner graphic assignment remains structurally accepted but has no
  corpus specimen.
