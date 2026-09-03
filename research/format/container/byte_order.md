# Byte order

**Covers:** How the reader establishes byte order per epoch, the Coda-banner `PC` platform token, and the still-unfound header byte-order marker.
**Read when:** Adding or changing container classification, or debugging a file that classifies as an unknown variant.
**Confidence:** confirmed for the compressed eras; structural but circumstantial for uncompressed; the header marker is open.

## Determining byte order

The reader does not read the byte order from anywhere. It trials both orders against the container
framing and keeps whichever validates: for the uncompressed era, a block chain that consumes the
file exactly and yields types 1, 2, 3, 4 in order; for the compressed eras, a first block type that
matches the era plus successful decompression and a matching CRC-32 for every block. The compressed
test is strong evidence. The uncompressed test is structural but circumstantial, and the
Coda-banner era is not detected at all, only asserted, because it has no block framing to trial.

**Found for the Coda-banner era: the product field names the platform.** That era's Windows
documents carry the banner product `PC 1.0+`, where its Mac documents carry a bare version —
`1.0.0`, `1.8.7`, `2.0.1`, `2.6`. The `PC` prefix is a statement in the file rather than an
inference from it, and it discriminates perfectly: 24 Windows documents carry it and are
little-endian, 252 Mac documents of the same era do not and are big-endian, and no other product
string in either corpus contains `PC` at all across 11,053 files.

**Test the leading `PC` token and nothing else.** What follows it is a version string and must not
participate: `1.0+` is the only one observed, but the whole point of the token is that the platform
is stated separately from the version, and matching `PC 1.0+` whole would reject a Windows document
from any other release for no reason. The `+` also means this is not a version the reader can parse,
so such a file will carry no recovered version at all and every version-gated mapping will be
skipped for it. That is survivable precisely because the era's mappings are gated on the epoch
rather than on a version.

This is what the paragraph below was looking for, for the one era that most needed it, and it
arrives from the banner rather than from the uninterpreted header bytes it guessed at. It also
replaces the classifier's numeric-product test, which is what currently rejects these files: the era
has a product that is deliberately not a version number.

**Still suspected but not found for the 3.x era onward: a byte-order marker in the header.** A format that is
written on two platforms with opposite byte order, as the 3.x line demonstrably is, would ordinarily
record which one it used rather than leaving a reader to infer it. The header holds candidates: the
six bytes at `0x062` are still uninterpreted, and the Coda-banner era carries a word at `0x80`
described below. Finding such a marker would replace a heuristic with a fact, and
would also give the Coda-banner era a real test in place of its current assertion. This is **open**.

The header version tuples are a second consumer of the answer, and they fail quietly without it.
Each is a 32-bit value in the file's own byte order, so decoding one requires knowing that order
already. `header::decodeVersion` accepts a byte order and only guesses when given `Unknown`, by
trying big-endian and keeping the result if its major version falls inside Finale's 0-27 range.
That test is far weaker than it looks, because a swapped value frequently lands inside the range
too. A Windows Finale 3.0 document stores `0f 03 01 03`: big-endian that reads as major 15, which
passes, while the correct little-endian reading is `0x0301030f`, version 3.0.1 build 15. Its Mac
counterparts store `03 01 03 0b`, the same version at a different build. The reader is unaffected —
the container establishes the order before the header is described — but anything that describes a
header on its own gets a plausible wrong answer with no indication, which is worth knowing when
writing a probe.

**Half the answer is already in the header, and it is a fact rather than a heuristic.** The platform
strings at `0x074` and `0x09a` are text, so they are byte-order independent and readable before any
order is chosen. A document that originated on Windows **must** be little-endian: Finale's Windows
releases ran on x86, which has no big-endian mode, so there is no such thing as a big-endian
Windows Finale file. A `WIN` platform string therefore settles the question outright.

The relationship is asymmetric, and only the Mac side is ambiguous. Macs were big-endian on PowerPC
and little-endian on Intel, so `MAC` says nothing on its own. The corpus shows exactly that shape:

| Platform | Order | Files | Products |
|---|---|---:|---|
| `WIN` | LE | 20 | 3.0, 2000, 2001, 2002, 2004b, 2005, 2006, 2007, 2012 |
| `MAC` | BE | 628 | 3.2–97, 2000–2006, 2007, 2008 |
| `MAC` | LE | 439 | 2007, 2008, 2009, 2010, 2012 |
| absent | BE | 62 | 1.8.7, 2.0.1, 2.6 |

No Windows document in the corpus is big-endian, across nine products spanning Finale 3.0 to 2012.
The Mac split begins at Finale 2007 and genuinely overlaps — 2007 and 2008 appear in both rows —
which is the Intel transition rather than a format change.

A `WIN` platform string can therefore replace the trial for that whole population, including the
uncompressed era where the trial is only circumstantial.

**Windows Coda-banner documents exist, are little-endian, and the reader rejects them. Confirmed
2026-08-11 from specimens.** The Finale 2.2 for Windows install disks yielded 24 `.MUS` files —
eight templates and sixteen tutorial documents — and they settle every part of this at once.

They carry a previously unseen banner *product*, not a new spelling: `Finale(TM) PC 1.0+ Copyright
1987 by Coda. All rights reserved.` uses the same `Finale(TM)` spelling the Mac Coda-banner era
uses, but its product field is `PC 1.0+` rather than a version number. The distinction matters,
because the banner parser recognises the spelling perfectly well and it is the numeric-product test
that rejects these files. Their pool prologue reads
`pages=20, pagesize=512` little-endian and nonsense big-endian, and the first record at `0x208` is
`fe ff 31 30`: comparator `0xfffe`, the globals comparator, followed by the tag `01`, both
little-endian. The record model is therefore identical to the Mac era of the same period and only
the byte order differs.

Two independent things stop the reader from opening one, and both are confirmed by these files. The
container's classifier requires `hasNumericProduct()`, which `PC 1.0+` fails, so such a file never
reaches the Coda-banner parser at all. Past that, `parseCodaBanner` asserts big-endian and would
break at its first prologue check, because the page-size word does not read `0x200` in that order.

The rest of this note is what was known before those specimens arrived, and is left as the reasoning
that predicted them.

 Every one of its 62 corpus files is big-endian and none carries a platform string, but
Finale for Windows existed during that era: Microsoft Knowledge Base article Q107181, the README for
Windows Sound System 2.0, records that "Finale 2.2 for Windows from Coda Music Technology is
compatible with the Windows Sound System sound board and software if you modify the WIN.INI file",
and mentions MusicProse for Windows alongside it. Finale 2.2 sits inside this era's 1.8.7-to-2.6
range.

A little-endian Coda-banner document is therefore a real possibility rather than a hypothetical one,
and the container would misread it: `parseCodaBanner` hardcodes big-endian, so such a file would
decode with transposed words and tags that are not text, most likely presenting as an unrecognized
variant rather than as wrong data. No such file is in any surveyed corpus, so nothing is currently
misread.

The era does carry a usable test, which is what makes this fixable rather than merely recordable.
Each pool prologue holds a page-size word that reads as `0x200` in one order and `0x0002` in the
other, so the order can be trialled against the prologue instead of asserted. That is recorded as a
gap in [PRODUCTION_READINESS.md](../../state/PRODUCTION_READINESS.md).

Source, accessed 2026-08-11:
[Microsoft KB Q107181, "Contents of the Windows Sound System 2.0 README.TXT", revised 13 June 2001](https://jeffpar.github.io/kbarchive/kb/107/Q107181/).
This is `public-source-derived`: it dates Windows Finale to version 2.2 but says nothing about that
version's file format or byte order.

**The Mac side is determined too, for every era that matters here.** Finale 2007 was the first
Intel-native release. MakeMusic announced at NAMM in January 2006 that "the next version of Finale
... will be made as a Universal Binary, running natively on both PowerPC and Intel-based Macs", and
Finale 2007 shipped that August with "support for Intel-based Macs with a Universal binary
release". Finale 2006 and earlier were PowerPC binaries, which on an Intel Mac ran under Rosetta —
emulated PowerPC code, and therefore still writing big-endian regardless of the hardware underneath.

So a Mac-origin document saved by Finale 2006 or earlier must be big-endian, and only Finale 2007
and later can be either. The corpus matches exactly: every `MAC` little-endian file is 2007 or
later, and the 2007 and 2008 overlap is a user population still mostly on PowerPC rather than a
format difference.

Combining the two platform facts leaves nothing circumstantial before Finale 2007:

| Origin | Era | Order |
|---|---|---|
| Coda banner, product `PC ...` | Windows, stated in the banner product | little-endian |
| Coda banner, numeric product | Mac; the era has no other platform | big-endian |
| `WIN`, any era | x86 has no big-endian mode | little-endian |
| `MAC`, Finale 2006 or earlier | no Intel-native build existed | big-endian |
| `MAC`, Finale 2007 or later | universal binary; follows the machine | either |

The uncompressed era spans Finale 3.0 to 2000, so it falls entirely in the rows that a platform
string determines, and its "structural but circumstantial" trial now has a fact behind it rather
than only a pattern. The Coda-banner row is the exception: it is still an assertion, because that
era records no platform. The one
genuinely ambiguous population, Mac documents from Finale 2007 onward, is also the one where the
trial is strongest, because the compressed eras validate every block against a stored CRC-32.

This does not remove the value of finding a real marker, which would still let the container decide
before it has parsed anything and would cover a file whose platform string is missing or wrong. But
it does mean no era currently depends on a guess.

Sources, accessed 2026-08-11:
[NAMM announcement, Macworld, 18 January 2006](https://www.macworld.com/article/178521/finale-8.html);
[Finale 2007 release, Macworld, 6 August 2006](https://www.macworld.com/article/181055/finale-9.html);
[Finale 2007 release, MacTech, 7 August 2006](https://www.mactech.com/2006/08/07/makemusic-releases-finale-2007-music-software/).
These are `public-source-derived` and corroborated by the corpus rather than independently verified
against MakeMusic's own release notes, which are no longer online.

### The word at 0x80, and the absent application version

**Confirmed across 138 files.** The Coda-banner header is entirely zero from `0x60` to `0x200`
except for one 16-bit field at `0x80`. It was previously recorded as a constant `01 03`; it is not
constant, and its value groups the era exactly:

| Product | Bytes at `0x80` | Files |
|---|---|---:|
| Finale 1.0.0 | `01 00` | 22 |
| Finale 1.8.7, 2.0.1, 2.6 | `01 03` | 92 |
| Windows, banner product `PC 1.0+` | `00 07` | 24 |

Those three pairs are the only ones in the corpus: all 234 distinct Coda-era documents take one of
them.

**It is not the application version.** It cannot be: it takes the same value for 1.8.7, 2.0.1 and
2.6, three products that differ. What it does track is the file format — one value for Finale 1.0.0,
another for the Mac releases after it, a third for Windows — which is consistent with the Windows
banner declaring a format of `PC 1.0+` rather than an application version.

**The best-fitting reading is that `0x81` is a flags byte and `0x04` marks the Windows build.** Its
values accumulate rather than enumerate: `0x00` for Finale 1.0.0, `0x03` for the Mac releases after
it, `0x07` for Windows. Read that way each release sets a further bit, and exactly one bit separates
Windows from the Mac line of the same period.

This is a better fit than treating `0x80` as the platform byte, for two reasons. First, that byte is
`1` on Mac and `0` on Windows, and Mac came first — a platform marker introduced when the second
platform arrived should leave the original at zero, not the newcomer. Second, if the field is a
16-bit word at all then it is subject to byte order, and `0x80` is therefore the high byte on Mac
and the low byte on Windows: comparing the two compares different halves of the field, so the
`1`-versus-`0` pattern is an artefact of the comparison rather than a finding.

What `0x04` actually means is **open**, and one specimen cannot separate the possibilities: with a
single Windows value observed, "little-endian", "PC build" and "the next feature bit, which happened
to arrive in the PC release" all predict the data equally. A Mac Coda document with the bit set, or
a Windows one without it, would distinguish them. Nothing reads this field — the banner's `PC` token
states the platform outright — so it is recorded rather than relied on.

**The field does not survive the Finale 3.0 redesign.** Across 1,500 signature-era files from
Finale 3.0 to 2012, `0x80` through `0x8b` is a twelve-byte run of zeros in 1,495 of them. That is
because the signature header states explicitly what the Coda word encoded implicitly: a Finale 3.7
document carries its version tuple at `0x6c`, the application `FIN` at `0x70`, the platform `MAC` or
`WIN` at `0x74`, an application version at `0x78` and a file version at `0x7c`, and the created block
ends exactly where the Coda field used to be. The modified block then begins at `0x8c`, leaving the
intervening twelve bytes unused.

Two documents out of roughly a thousand distinct signature-era files carry a single stray byte at
`0x80` — `01` in a Finale 2000 document and `03` in a Finale 2003 one, with `0x81` onward still zero.
Neither reproduces a Coda pair, so a stale remnant of an upgraded document is a guess rather than an
explanation, and the cause is **open**. Nothing reads the byte, so nothing depends on it.

**No application version is stored anywhere in these files.** A search of all 24 Windows documents
for the version their application reports, 2.2, found no encoding of it — not `0x0202`, `0x0220`,
decimal 22 or 220, nor the text `2.2` — at any offset common to all of them. Their exact Finale 27
companions corroborate it from the other side: every one carries a `<modified>` block naming Finale
27 itself and **no `<created>` block at all**, so Finale 27 recovers no creator version from them
either. For this era the banner product is the only version there is, which for Mac documents is the
application version and for Windows documents is a format marker that is not one.
