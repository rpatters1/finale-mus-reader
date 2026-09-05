# StemOptions investigations

**Covers:** Experiments behind stem-option recovery and companion transformations.
**Read when:** Investigating stem scalars, stem connections, or their character encoding.
**Confidence:** each entry states its own result.

## 2026-09-05 — Windows-font stem byte converted as MacRoman

- **Question:** Is the `192` to `191` stem-symbol disagreement a reader decoding error or a
  Finale conversion error?
- **Source bytes:** `mus-d4cddb217051faf7` stores connection 0 as font comparator 2 and symbol
  byte `0xc0`. Font comparator 2 names Times and states Windows charset bank, charset 0.
- **Companion bytes:** The companion explicitly writes symbol 191 and retains font comparator 2
  as Times with Windows charset bank, charset 0. U+00BF is the MacRoman decoding of byte `0xc0`;
  Windows-1252 decodes it as U+00C0.
- **Result:** The source-directed value 192 is retained. The exact Mac-origin, uncompressed,
  Times-font conversion is classified as `text-encoding-error`. This is **weak**, based on one
  source, and does not authorize treating other character disagreements as the same conversion.
