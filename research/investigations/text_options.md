# TextOptions investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-09-02 — Finale 3.7 TextOptions insert boundary

- **Source:** The repository owner inspected the Finale 3.7 addendum and confirmed that configurable
  text inserts appeared in the UI in Finale 3.7. Page-level bibliographic details have not yet been
  recorded, so this remains a user-supplied manual fact rather than a public citation.
- **Consequence:** This closes the introduction boundary independently of the bytes. Selector `78`
  is absent through every observed Finale 3.5 document and present from Finale 3.7 onward; the
  addendum establishes that the change represents a new UI feature rather than merely a record
  relocation.
- **Remaining question:** The 17-byte Finale 3.7--2000 physical mapping remains strong rather than
  confirmed because no controlled save directly verifies its two 32-bit widths, and no Windows
  specimen distinguishes always-little-endian storage from storage opposite the container order.
