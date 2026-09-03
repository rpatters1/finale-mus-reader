# GraceNoteOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-29 — Coda GraceNoteOptions width held at the pinned default

- **Question:** whether the Coda companion's varying `graceSlashWidth` can be recovered from
  a legacy general line-width record or from the selector used by later GraceNoteOptions.
- **Controlled exclusion:** changing Finale 1.0.0 Def Lin Width from 0.5 to 0.625 changes only
  selector `54` word 0, as single-precision 0.5 to 0.625; the ETF repeats the edit and the
  companion retains `graceSlashWidth` 224. Selector `54` is therefore not imported as the
  grace-specific width.
- **Uncontrolled correlation:** the fifteen tracked Finale 2.6.3 documents pair selector `64`
  word 1 values 2500 and 3249 with companion widths 64 and 83. The values admit a
  ten-thousandths-of-a-point conversion, but the documents are not a controlled edit of this
  setting and the correlation is not implemented.
- **Decision:** only `gracePerc` is sourced from Coda records. The other five fields keep the
  pinned Finale 27 baseline values and report as `Finale27Default`. Only a differing
  `grace_note_options.grace_slash_width` leaf in the Coda epoch with that origin is classified
  as `different_defaults`; all other paths, epochs, and origins remain strict.
- **Tracked result:** all 160 sources and all 160 companions complete. GraceNoteOptions has
  912 equal leaves, 48 `different_defaults`, and no unexpected differences. The five unsourced
  Coda fields are intentionally pinned defaults; no all-corpus probe was run.
- **Full-corpus result:** the subsequent authorized three-survey capture selects 16,257
  occurrences representing 7,223 distinct source ids. It imports 16,168 occurrences
  representing 7,150 distinct ids, and all 4,568 companion-backed occurrences representing
  2,983 distinct ids compare successfully. GraceNoteOptions has 27,260 equal leaves, 148
  Coda-width `different_defaults`, and no unexpected differences. The expected widths span
  product labels 1.0.0 (55), 2.6 (69), and versionless Windows PC 1.0+ (24), and all three
  surveys contribute to the population. The 89 failed source occurrences are outside the
  companion-comparison totals.
