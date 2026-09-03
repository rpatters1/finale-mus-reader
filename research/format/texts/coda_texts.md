# Coda-banner text

**Covers:** How the Finale 1.x-2.6 epoch stores block text in the `HT`/`HS` others families and its shorter binary command form.
**Read when:** Working on text in the Coda-banner epoch.
**Confidence:** strong; see [text_blocks.md](../others/text_blocks.md) for the block record itself.

## The Coda-banner epoch

Its two length-prefixed text chunks are empty in every fixture. Its block text is instead in
the `HT` others family, paired with style and insert information in `HS`. Its own binary
commands use a shorter argument form than the later epochs (`^\x82\x40\x81`), so the digit
rule above does not apply to it.
