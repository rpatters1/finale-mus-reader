# Sharing investigations

**Covers:** Dated experiment entries behind the findings in the corresponding reference document.
**Read when:** Investigating this subject, before proposing a new hypothesis about it -- the experiment may already have been run.
**Confidence:** each entry states its own result; refuted predictions are kept deliberately.

## 2026-08-05 — Sharing census

- **Question:** Is sharing visible and localized?
- **Method:** count `partDef`, `part`, and `shared` in decoded Finale 27 XML.
- **Observation:** 301 multi-part documents, 315 with part-scoped objects, 2,188,767 part-scoped elements, and both true/false shared attributes. `0x011a` maps to `partDef`.
- **Conclusion:** sharing is pervasive within ordinary record types rather than visibly isolated in a named XML section. Legacy duplication versus references remains unresolved.

## 2026-08-21 — Zlib class-record part dimension

- **Question:** Does the fourth header word of a zlib detail record identify an incidence or
  the source part?
- **Method:** Compared all class `0x041d` headers and payloads in two Finale 2008 documents
  with the raw `measGraphicAssign` nodes in their independently decoded Finale 27 companions.
- **Observation:** Each source has nine records whose header value is zero and three whose
  value is 17. The latter repeat the complete score payloads for the same staff and measures.
  Each companion has corresponding empty nodes with `part="17" shared="true"`; every node's
  XML incidence is zero. Header and trailer bytes otherwise match their score counterparts.
- **Conclusion:** **Confirmed for `0x041d`.** The header field is the part id. Incidence remains
  structural within the class payload. Current import remains deliberately score-only; linked
  part reconstruction and sharing-mode inference are deferred.

## 2026-09-03 — Zlib part ownership and structural sharing

- **Question:** Do zlib class-record headers directly encode EnigmaXML identity attributes, and
  can part sharing be recovered from record structure without a parallel policy model?
- **Controlled evidence:** `F2012-noteartexp.mus` contains only the score record for class
  `0x00b1`, cmper 1. Unlinking the expression creates a part-1 record with an unchanged complete
  primary snapshot and a same-sized continuation whose edit-specific mask group is zero. Moving
  that unlinked expression 24 EVPU changes its primary horizontal offset and that group to
  `0xffff`. The Finale 27 companions represent both part records as `shared="true"`.
- **Cross-class comparison:** Continued part records for page-text assignments and smart shapes
  likewise correspond to `shared="true"`. Full standalone page and staff-system records without
  continuations correspond to `shared="false"`. `measSpec` instead uses a 26-byte score tuple and
  an 8-byte compact part tuple and corresponds to `shared="true"`.
- **Implementation:** Normalized zlib rows now retain part id, continuation bytes, both terminal
  words, and distinct physical and effective payload views. One structural classifier returns
  musxdom's existing `ShareMode`: score is `All`, a continued or importer-described compact part is
  `Partial`, and another standalone part is `None`. For every continued part row, the row pool
  begins the effective payload with the equal-sized score payload and applies the continuation as
  a bit mask selecting stored part bits. Every importer reads this effective view before parsing,
  so no class-specific overlay callback is required. All current zlib others/detail import paths
  enumerate actual header part ids. Compact layouts remain score/part payload-size pairs owned by
  each class importer; the `measSpec` 26/8-byte geometry exercises classification until its
  importer is implemented.
- **DOM boundary:** musxdom no longer retains per-field XML child names through
  `getUnlinkedNodes`; no repository or MuseScore2 Finale-import consumer used them. Its
  `PartSharingFactory` remains available for a future compact-layout importer, but continued
  records are resolved generically as bytes before DOM construction.
- **Bit-mask evidence:** The expression fixtures show a moved word changing its aligned mask from
  zero to `0xffff`. Continued page-graphic records show bit `0x0010` selecting `hidden`
  independently from the low-nibble page selection stored in the same word. The generic bit merge
  removes all 23 formerly unexpected page-assignment leaves in the broad corpus without a page or
  measure callback.
- **Validation and confidence:** The final tracked capture imported 220 sources and 220 companions
  with no unexpected differences. The final three-survey capture selected 16,317 occurrences;
  16,228 imported, all 4,628 available companions imported, and no comparison was unexpected. The
  89 pre-comparison failures remain 58 Finale LIB files and 31 inputs that do not classify as MUS.
  No continued part row used by an importer lacked a structurally compatible score row. Header
  identity, structural sharing, and the continuation bit overlay are therefore **strong** across
  the observed zlib classes. Compact partial field layouts remain class-specific and open until
  their importers are implemented.
