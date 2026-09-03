# Failed and revised hypotheses

**Covers:** Interpretations that were tested and disproved, kept so they are not retried.
**Read when:** Before proposing a hypothesis about framing, record width, byte order, or the 2001-2006 payload.
**Confidence:** disproved -- every entry here is a dead end, recorded deliberately.

## Failed or revised hypotheses

- Records do **not** start at offset zero; the common body boundary is `0x200`.
- The 2001–2006 fixed physical records are **16 bytes, not 16 words**. The remembered 12/10 payload capacities are
  bytes, so the proposed two unexplained words were an artifact of mixed units.
- The formerly “low-entropy/encoded” Finale 3.x–2000 family is **not encoded or compressed**: it contains the same
  fixed physical rows directly in four typed pools, with platform-dependent byte order.
- DCL block type `0x0012` is **not inherently a terminal marker**; it is compressed and nonempty in many documents.
  A six-byte block of the next sequential type marks the first empty/end pool.
- 2007+ principal records are **not** fixed at 16 words or 32-byte aligned.
- Treating all zlib members as the generic record pool failed; only `0x001a` and `0x001b` fit, while entries and texts use other layouts.
- Byte order is **not** derivable from `MAC` versus `WIN` alone during the 2007–2008 transition.
- XML counts are not always binary counts; Finale 27 expands or normalizes `frameSpec`, smart shapes, text definitions, details, and part-scoped data.
- The 2001–2006 high-entropy payload is **not** an unknown transform or encryption; it is standard PKWARE DCL. Tests
  that considered only zlib missed it.
