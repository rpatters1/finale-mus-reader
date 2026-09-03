# Entry pool

**Covers:** Entry row widths by epoch and entry pool framing.
**Read when:** Modifying entry decoding or debugging note/rest conversion.
**Confidence:** confirmed for row widths; field interpretation is largely open.

## Entry pool

**Finale 2001–2006 framing solved; fields partly mapped.** Every nonempty decoded `0x0011` pool in the directly
resolved corpus is a sequence of 38-byte rows. The controlled documents contain three rows matching three ETF `eE`
records; the first four bytes are the big-endian entry number, and the controlled C-to-D edit changes the note pitch
inside the corresponding row while preserving its size. The public PDK confirms that entry numbers and entry/note
flags are 32-bit and several duration/count fields are 16-bit, but its 146-byte plug-in API structure includes
expanded note capacity and computed fields and must not be mistaken for the 38-byte disk row. Exact raw field and
flag mapping is now a bounded correlation task.

For Finale 2007+, block `0x0016` decompresses cleanly and CRC-validates but does not use either the earlier 38-byte
row or the later generic zero-trailed frame. Its decoded size tracks document complexity; that era remains open.
