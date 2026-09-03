# Public Finale 2000 PDK evidence

**Covers:** The public PDK material consulted, its provenance, and what it establishes.
**Read when:** Citing a PDK-derived fact, or checking whether one has been independently verified.
**Confidence:** public-PDK-derived until independently binary-verified.

## Public Finale 2000 PDK evidence

**Public-PDK-derived, with the physical framing independently binary-verified.** On 2026-08-08 the project adopted
the public-source provenance policy in the README and consulted the Finale 2000 PDK copy included in GRAME's public
GUIDOLib repository at immutable commit
[`9f74ba9b3e287f240bbd454c2259fc3f7737c6ad`](https://github.com/grame-cncm/guidolib/tree/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin).
The included README explicitly identifies the material as the Finale 2000 PDK. No PDK file is stored in this
repository.

The PDK data API supplies three decisive factual concepts:

1. An extended data tag is a 32-bit value whose high 16 bits select a storage class and whose low 16 bits are the
   two-character Enigma tag.
2. Ordinary “other” IDs use one 16-bit comparator; detail IDs use two; entry IDs use a 32-bit entry number. An
   incident is an API selection/order dimension, not an additional field found in the fixed physical row.
3. Logical structures larger than one physical payload are stored across a declared number of successive
   incidences. Strings, arrays, and specially handled structures use separate storage classes.

These concepts explain both the two-character tags in the decoded pools and why records with the same tag/key recur
in ETF. They also warn against treating every physical row as a complete musxdom object. For example, the Finale 2000
definitions occupy two physical rows for `MS` (measure attributes), two for `Iu` (staff-list membership), two for
`PS` (page layout), two for `SS` (staff-system layout), and three for `IS` (staff attributes). The controlled ETF
evidence shows later expansion without changing the tag: `IS` grows from three rows in Finale 2002 to six in Finale
2003, while `MS` and `SS` grow from two rows through Finale 2004 to three in Finale 2005. A decoder must therefore
select a versioned logical layout after reading stable physical rows.

Public source files consulted, access date 2026-08-08:

- [`edata.h`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/edata.h): storage classes, IDs, tags, and ordinary logical structures;
- [`EEDDATA.H`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/EEDDATA.H): entry-detail tags and structures;
- [`EXTYPES.H`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/EXTYPES.H): entry/note API types and PDK version history;
- [`VERSION.H`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/VERSION.H): primitive widths and tag construction; and
- [`FINEXTND.H`](https://github.com/grame-cncm/guidolib/blob/9f74ba9b3e287f240bbd454c2259fc3f7737c6ad/platforms/win32/finale-plugin/FINEXTND.H): versioned data API behavior.
