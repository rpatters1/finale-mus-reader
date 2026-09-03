# Fret instruments, groups, styles, and diagrams

**Covers:** FretInstrument, FretboardGroup, FretboardStyle, and FretboardDiagram record identities and layouts.
**Read when:** Working on any fret or fretboard record.
**Confidence:** public-PDK-derived, corpus-checked; see the confidence notes inline.

## Fret instruments, groups, styles, and diagrams

**Confirmed for the DCL and zlib epochs.** The four source-owned record classes are:

| musxdom class | Pool | Fixed tag | Class id | Payload |
|---|---|---|---:|---|
| `FretboardGroup` | others | `fg` | `0x0094` | 60-byte tuples before Finale 2012; 204-byte tuples from Finale 2012 |
| `FretInstrument` | others | `fI` | `0x0095` | 60-byte header/name plus one word per string |
| `FretboardStyle` | others | `ft` | `0x0097` | 156 bytes |
| `FretboardDiagram` | details | `fb` | `0x0413` | 10-byte header plus padded item incidences |

`FretboardGroup` keeps the chord-suffix comparator. Before Finale 2012, each group incidence is a
60-byte tuple: instrument comparator at offset 0 and a 48-byte platform-encoded name at offset
12. Finale 2012 widens that name to 96 UTF-16LE code units, making each tuple 204 bytes while
leaving the instrument comparator and name offset unchanged. `FretInstrument` stores the
diatonic-fret bitset at offset 0, fret and string counts at
offsets 4 and 6, Speedy Entry clef at offset 8, a 48-byte platform-encoded name at offset 12,
and one signed pitch word per string from offset 60. The bitset stores its low word before its high
word independently of container byte order. String pitches retain little-endian byte order even in
the big-endian fixed and class records exercised by the tracked fixtures. Set bit *n* in the
diatonic mask names fret *n + 1*; a zero mask is the chromatic spelling. The pre-25.3 public
Framework accessor confirms that each old string element is only the pitch word, so musxdom's
`nutOffset` remains zero.

Finale 25.3 replaced that old pitch word with separate byte-sized pitch and nut-offset members,
after the last MUS format. Finale 27 can reinterpret a stale or corrupt old tuning word as that
new pair during upgrade: old signed word `0x5089` becomes pitch `0x89` and nut offset `0x50`, for
example. Coverage recognizes the transformation only when the companion's two byte-sized leaves
exactly reconstruct the source's 16-bit word. The recovered pitch remains source-owned; the zero
nut offset is `LegacyBehavior` because no MUS record stored that member.

The fixed-row representation concatenates the payload of each incidence into the same byte layout
used by the later class record. A group tuple occupies five other incidences, an instrument with six
strings occupies six, and a style occupies thirteen. The diagram header is detail incidence 0; each
following detail incidence contains two four-byte cells or barres plus one padding word.

`FretboardStyle` follows the public Framework `__FCFretStyle` layout. Its ten Efix values and two
fingering offsets are stored as high-word/low-word pairs even in a little-endian zlib file; the
words themselves still follow container byte order. The record includes 48 bytes for the style
name and 24 for the fret-number label.

`FretboardDiagram` has five header words: fret count, starting fret, flags, cell count, and barre
count. Flag `0x0001` locks the diagram and `0x0004` shows its number. A cell or barre is two words,
but each physical detail incidence holds only two items followed by one padding word. Cells and
barres begin separate incidence arrays, so an odd cell count leaves the rest of its incidence
unused before the first barre. Its second comparator packs the fretboard-group incidence in the
high bits and the chromatic root index in the low four bits; the public Framework implementation
is `16 * fretboardGroupInci + rootIndex`.

The one-fixture Release capture for `mus-21b30fb5dfc9bca2` found exactly the same instance sets on
both sides: no fret instance was reader-only or companion-only. All 510 instrument leaves, all 465
group leaves, and all 55,373 diagram leaves agreed. Style numeric fields agreed as well. Three
multiword style names differ only because musxdom's current companion XML mapping reads
`FretboardStyle::name` through a whitespace-token conversion; direct inspection of `score.dat`
shows the full names agree with the legacy bytes. The importer preserves the complete source name.

A separate Finale 2001 DCL capture (`mus-46c4619dfdc99ae6`) agreed on all 459 group leaves and all
54,601 diagram leaves. Its two instruments agreed on 32 companion leaves; each also retains one
source diatonic-fret value that Finale 27 omitted during conversion. Style numeric leaves agreed,
with the same three whitespace-tokenized companion names described above. Thus the fixed import
also preserves stored values that a modern companion normalizes away.

The introduction boundary is **strong**, not used as a decoder gate. An exhaustive scan of the
filesystem-origin `rpatters1-main` files found none of the four tags in 186 Finale 2000 paths (149
distinct contents). Four distinct Finale 2001 documents contain all four tags; the fifth distinct
Finale 2001 content is a `.FAN` file containing none. The importer searches every fixed-row epoch
for the identities themselves, so an earlier document produces no objects without depending on a
recovered marketing version. The hypothesis that pre-2001 chord fretboards were rendered only as
Seville-font glyphs remains **open**; absence of these editable records does not establish the older
display mechanism.

The 125-document tracked-evidence capture spans 33 Coda-banner, 25 uncompressed, 44 DCL, and 23
zlib sources, with all sources and companions readable. With no fret-specific expected-difference
rules, instruments have 2,968 equal leaves, one reader-only leaf, and 928 companion-only leaves;
groups have 1,556 equal and 280 unequal leaves; styles have 2,250 equal, 12 unequal, and 1,682
companion-only leaves; all 218,404 diagram leaves agree. The 58 pre-2001 companions each add
exactly one 16-leaf fret instrument and one 29-leaf fret style although the source contains none
of the four record identities. The reader continues to construct only stored instances.

Four big-endian Finale 2006 DCL companions byte-swap the instrument reference in later group
incidences: source values `1` and `2` become `256` and `512`. Incidence zero remains unchanged,
same-version ETF preserves the source values, and the companions define no fret instruments 256
or 512. The tracked comparison treats a recovered fretboard-group instrument reference as Finale
upgrade loss when a big-endian source and its companion differ by exactly a 16-bit byte swap; the
executable predicate is in `tools/coverage/comparison.cpp`. The same four companions reorder
the stored group name `Minor7 b5 R4 (copy)` as `Minor7 b5 R4( ocyp`; same-version ETF retains the
source name. Its eight-byte C-string tail changes from ` (copy)\0` to `( ocyp\0)`: Finale swaps
each adjacent byte, including the closing parenthesis with the terminator. Coverage classifies that
exact big-endian fretboard-group name transformation as Finale upgrade loss. The same operation
also covers a terminator in the first byte of a word: `Minor7 b5 R4\0t` becomes
`Minor7 b5 R4t\0`, exposing the byte that followed the stored C string. The 12
style-name differences exposed a musxdom mapping that used numeric token extraction for text. After
that mapping was corrected to read the complete XML content, the source and companion style names
agree. One Finale 2002 instrument retains a source diatonic-fret leaf that its companion omits; that
observation remains unclassified pending review.

The public Finale 2000 PDK snapshot contains no `EDT` declaration whose name contains `Fret`.
The current public Framework headers expose `__FCFretStyle`, `EDTFretInstrument`,
`EDTFretInstrument25_3`, `EDTFretGroup2012`, `__FCEDTFretItemsHeader`, and
`__FCEDTFretItem`. They give fixed-row tags `ft` for styles and `fb` for diagrams. The public
declarations leave `FCFretInstrumentDef::Tag()` out of line and name the group tag only as
`otx_FretGroup`; the `fI` and `fg` spellings are independently corpus-verified rather than
public-PDK-derived.

Public sources consulted on 2026-08-26:

- Finale 2000 PDK headers at immutable GUIDOLib commit
  [`9f74ba9b3e287f240bbd454c2259fc3f7737c6ad`](https://github.com/grame-cncm/guidolib/tree/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin);
- public Framework
  [`ff_other.h`](https://pdk.finalelua.com/ff__other_8h_source.html#l21888) and
  [`ff_details.h`](https://pdk.finalelua.com/ff__details_8h_source.html#l01683).
