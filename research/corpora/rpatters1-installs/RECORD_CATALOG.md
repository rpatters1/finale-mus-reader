# Record Catalog

This catalogs numeric identifiers observed after decompressing record-framed zlib blocks. It does not claim that pre-2007 encoded bytes use the same identifiers. The proposed XML names come from corpus-wide count correlation with Finale 27 exports; conversion differences and count collisions remain possible. All candidate rankings are preserved in [`data/record_correlations.csv`](data/record_correlations.csv), and per-version raw observations in `private/generated/<survey_id>/record_catalog.csv` (local-only).

Frame fields are described in [zlib_blocks.md](../../format/container/zlib_blocks.md). `Example offset` is relative to the decompressed zlib member, not the file.

## Finale 2000 PDK tag reference

This reference table records two-character tags identified from the publicly accessible Finale 2000 PDK at GUIDOLib commit `9f74ba9b3e287f240bbd454c2259fc3f7737c6ad`. The meanings are `public-PDK-derived`; every listed tag is also observed in the available ETF evidence. Rows marked binary-verified occur at the expected position in Finale 2000 or decoded 2002–2005 fixed rows. See [pdk_public_evidence.md](../../reference/pdk_public_evidence.md) for provenance and [zlib_blocks.md](../../format/container/zlib_blocks.md) for the independent framing checks.

| Tag | Logical meaning | Storage family | Verification |
|---|---|---|---|
| `CN` | notehead modification | entry detail | binary-verified in Finale 2000 |
| `DI` | separate score-expression placement | other | ETF-observed |
| `DO` | shape-expression definition | other | ETF-observed |
| `DT` | text-expression definition | other | ETF-observed |
| `DY` | score-expression assignment | other | ETF-observed |
| `ED` | staff-expression assignment | entry detail | ETF-observed |
| `GF` | frame holder | detail | binary-verified |
| `IM` | articulation assignment | entry detail | ETF-observed |
| `IS` | staff attributes | other | binary-verified |
| `Iu` | staff-list membership / staff used | other | binary-verified |
| `MN` | measure-number region | other | ETF-observed |
| `MS` | measure attributes | other | binary-verified |
| `NG` | staff-group attributes | detail | ETF-observed |
| `PD` | expression MIDI-dump playback data | other | ETF-observed |
| `PS` | page attributes | other | binary-verified |
| `SD` | shape definition | other | binary-verified |
| `SS` | staff-system attributes | other | binary-verified |
| `TP` | tuplet definition | entry detail | binary-verified in Finale 2000 |
| `TX` | text-block definition | other | ETF-observed |
| `pT` | page-text assignment | other | ETF-observed |
| `eE` | entry | entry pool | 38-byte row binary-verified |

## Finale 2007+ numeric identifiers

| Identifier | Proposed structure | Pool | Confidence | Versions | Payload bytes observed | Example (member:offset) |
|---|---|---|---|---|---|---|
| `0x000f` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x0) |
| `0x0010` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x1a) |
| `0x0011` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x34) |
| `0x0012` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x4e) |
| `0x0013` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x68) |
| `0x0014` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x82) |
| `0x0015` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x9c) |
| `0x0016` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xb6) |
| `0x0017` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xd0) |
| `0x0018` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xea) |
| `0x0019` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x104) |
| `0x001a` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x11e) |
| `0x001b` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x138) |
| `0x001c` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x152) |
| `0x001d` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x16c) |
| `0x001e` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x186) |
| `0x001f` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x1a0) |
| `0x0020` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 36 | `mus-72605d9c006cbd00` (0:0x1ba) |
| `0x0021` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x1ec) |
| `0x0022` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x206) |
| `0x0023` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x220) |
| `0x0024` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x23a) |
| `0x0025` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x254) |
| `0x0026` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 276 | `mus-72605d9c006cbd00` (0:0x26e) |
| `0x0027` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 24 | `mus-72605d9c006cbd00` (0:0x390) |
| `0x0028` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 180 | `mus-72605d9c006cbd00` (0:0x3b6) |
| `0x0029` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x478) |
| `0x002a` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x492) |
| `0x002b` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x4ac) |
| `0x002c` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x4c6) |
| `0x002d` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x4e0) |
| `0x002e` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x4fa) |
| `0x002f` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x514) |
| `0x0030` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x52e) |
| `0x0031` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x548) |
| `0x0032` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x562) |
| `0x0033` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x57c) |
| `0x0034` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x596) |
| `0x0035` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x5b0) |
| `0x0036` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 1536 | `mus-72605d9c006cbd00` (0:0x5ca) |
| `0x0037` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 36 | `mus-72605d9c006cbd00` (0:0xbd8) |
| `0x0038` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xc0a) |
| `0x0039` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xc24) |
| `0x003a` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xc3e) |
| `0x003b` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xc58) |
| `0x003c` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xc72) |
| `0x003d` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xc8c) |
| `0x003e` | unknown | unknown/options | open | 2008, 2009, 2010, 2011 | 84 | `mus-ec1ca5c7cf830615` (0:0xca6) |
| `0x0040` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xca6) |
| `0x0041` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xcc0) |
| `0x0042` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 24 | `mus-72605d9c006cbd00` (0:0xcda) |
| `0x0043` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 24 | `mus-72605d9c006cbd00` (0:0xd00) |
| `0x0045` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 60 | `mus-72605d9c006cbd00` (0:0xd26) |
| `0x0046` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 36 | `mus-72605d9c006cbd00` (0:0xd70) |
| `0x0047` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12, 24 | `mus-72605d9c006cbd00` (0:0xda2) |
| `0x0048` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12, 24 | `mus-72605d9c006cbd00` (0:0xdbc) |
| `0x0049` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xdd6) |
| `0x004a` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xdf0) |
| `0x004b` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xe0a) |
| `0x004c` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xe24) |
| `0x004d` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xe3e) |
| `0x004e` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xe58) |
| `0x004f` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xe72) |
| `0x0050` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xe8c) |
| `0x0051` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xea6) |
| `0x0052` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xec0) |
| `0x0053` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xeda) |
| `0x0054` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xef4) |
| `0x0055` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xf0e) |
| `0x0056` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xf28) |
| `0x0057` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xf42) |
| `0x0058` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xf5c) |
| `0x0059` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xf76) |
| `0x005a` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0xf90) |
| `0x005b` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 108 | `mus-72605d9c006cbd00` (0:0xfaa) |
| `0x005c` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 96 | `mus-72605d9c006cbd00` (0:0x1024) |
| `0x005d` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x1092) |
| `0x005e` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x10ac) |
| `0x005f` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x10c6) |
| `0x0060` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x10e0) |
| `0x0061` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x10fa) |
| `0x0062` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 48 | `mus-72605d9c006cbd00` (0:0x1114) |
| `0x0063` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 48 | `mus-72605d9c006cbd00` (0:0x1152) |
| `0x0064` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 60 | `mus-72605d9c006cbd00` (0:0x1190) |
| `0x0065` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 24 | `mus-72605d9c006cbd00` (0:0x11da) |
| `0x0066` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 120 | `mus-72605d9c006cbd00` (0:0x1200) |
| `0x0068` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 108 | `mus-72605d9c006cbd00` (0:0x1286) |
| `0x0069` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x1300) |
| `0x006a` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x131a) |
| `0x006b` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x1334) |
| `0x006c` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 36 | `mus-72605d9c006cbd00` (0:0x134e) |
| `0x006d` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 324 | `mus-72605d9c006cbd00` (0:0x1380) |
| `0x006e` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x14d2) |
| `0x006f` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 24, 36 | `mus-72605d9c006cbd00` (0:0x14ec) |
| `0x0070` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 48 | `mus-72605d9c006cbd00` (0:0x1512) |
| `0x0071` | unknown | unknown/options | open | 2008, 2009, 2010, 2011 | 84 | `mus-ec1ca5c7cf830615` (0:0x15be) |
| `0x0079` | articDef | others | strong | 2007, 2008, 2009, 2010, 2011 | 36, 48 | `mus-72605d9c006cbd00` (0:0x1550) |
| `0x007a` | measSpec | others | weak | 2007, 2008, 2009, 2010, 2011 | 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 168, 180, 192, 204, 216, 228, 240, 252, 264, 276, 288, 300, 348, 444, 468 | `mus-02eb545bb34c359c` (0:0x2454) |
| `0x007c` | channelPlayData | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-02eb545bb34c359c` (0:0x3888) |
| `0x007d` | shapeDef | others | weak | 2007, 2008, 2009, 2010, 2011 | 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 168, 192 | `mus-72605d9c006cbd00` (0:0x231e) |
| `0x007e` | chordSuffixPlay | others | strong | 2007, 2008, 2009, 2010, 2011 | 12, 24, 36, 48, 60, 72, 84, 96, 108, 144, 168 | `mus-72605d9c006cbd00` (0:0x3916) |
| `0x007f` | drumStaff | others | weak | 2007, 2008, 2009, 2010, 2011 | 24, 36, 48 | `mus-060e10ac5f90cc2d` (0:0x5c98) |
| `0x0083` | channelPlayData | others | weak | 2007, 2008, 2009, 2010, 2011 | 12, 24, 36, 48 | `mus-72605d9c006cbd00` (0:0x418c) |
| `0x0084` | drumStaff | others | weak | 2007, 2008, 2009, 2010, 2011 | 12, 24 | `mus-bb31fc25afbcf478` (0:0x45c2) |
| `0x0085` | drumStaff | others | weak | 2007, 2008, 2009, 2010, 2011 | 12, 24 | `mus-72605d9c006cbd00` (0:0x441a) |
| `0x0086` | durAllot | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x4466) |
| `0x0088` | execShape | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x46bc) |
| `0x008a` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-545b14c912206c49` (0:0x45c6) |
| `0x008b` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 24, 36, 48, 60 | `mus-545b14c912206c49` (0:0x479a) |
| `0x008c` | fretboardSymbol | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-bb31fc25afbcf478` (0:0x48f2) |
| `0x008d` | layerAtts | others | weak | 2007, 2008, 2009, 2010, 2011 | 12, 36, 48 | `mus-72605d9c006cbd00` (0:0x47f4) |
| `0x0090` | fontName | others | weak | 2007, 2008, 2009, 2010, 2011 | 24, 36, 48 | `mus-72605d9c006cbd00` (0:0x4874) |
| `0x0092` | frameSpec | others | weak | 2007, 2008, 2009, 2010, 2011 | 12, 24 | `mus-72605d9c006cbd00` (0:0x4a2e) |
| `0x0093` | lockMeas | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-1ee495414f3aab74` (0:0x2078) |
| `0x0094` | shapeDef | others | weak | 2007, 2008, 2009, 2010, 2011 | 60, 120, 180, 240, 300, 360, 420, 480, 540, 600, 660, 720, 780, 840, 900, 960, 1080, 1200, 1260, 1440, 1500, 1680, 1800, 1920, 2100, 2160 | `mus-72605d9c006cbd00` (0:0x4a48) |
| `0x0095` | fretInst | others | strong | 2007, 2008, 2009, 2010, 2011 | 72, 84 | `mus-72605d9c006cbd00` (0:0x72bc) |
| `0x0097` | fretStyle | others | strong | 2007, 2008, 2009, 2010, 2011 | 156 | `mus-72605d9c006cbd00` (0:0x7312) |
| `0x009a` | unknown | unknown/options | open | 2007, 2008 | 24 | `mus-1ee495414f3aab74` (0:0x2402) |
| `0x009b` | unknown | unknown/options | open | 2007, 2008 | 24 | `mus-1ee495414f3aab74` (0:0x2428) |
| `0x009c` | measNumbRegion | others | weak | 2007, 2008, 2009, 2010, 2011 | 12, 24 | `mus-060e10ac5f90cc2d` (0:0xbdc8) |
| `0x009e` | playDefs | others | weak | 2007, 2008, 2009, 2010, 2011 | 36 | `mus-72605d9c006cbd00` (0:0x7664) |
| `0x009f` | staffSystemSpec | others | moderate | 2007, 2008, 2009, 2010, 2011 | 24, 48, 72, 96, 120, 144, 168, 192, 216, 240, 264, 288, 432, 456, 480, 504, 600, 624, 648, 672 | `mus-72605d9c006cbd00` (0:0x7696) |
| `0x00a0` | unknown | unknown/options | open | 2007, 2008 | 12 | `mus-1ee495414f3aab74` (0:0x24e4) |
| `0x00a2` | keysAttrib | others | strong | 2007, 2008, 2009, 2010, 2011 | 192, 216 | `mus-e217b8fd4aa4432d` (0:0x9028) |
| `0x00a3` | layerAtts | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x76fa) |
| `0x00a4` | measNumbRegion | others | strong | 2007, 2008, 2009, 2010, 2011 | 96, 204 | `mus-e217b8fd4aa4432d` (0:0x925c) |
| `0x00a5` | metaArtic | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x7762) |
| `0x00a6` | metaChord | others | moderate | 2007, 2008, 2009, 2010, 2011 | 12, 24 | `mus-1ee495414f3aab74` (0:0x26f2) |
| `0x00a7` | metaClef | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x7b0a) |
| `0x00a8` | metaDynam | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x7caa) |
| `0x00a9` | metaKeySig | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-e217b8fd4aa4432d` (0:0x9b86) |
| `0x00aa` | metaRepeat | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x7f82) |
| `0x00ab` | metaShape | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-e217b8fd4aa4432d` (0:0x9f7c) |
| `0x00ac` | metaStaffStyle | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x8086) |
| `0x00ad` | metaTimeSig | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-e217b8fd4aa4432d` (0:0xa33e) |
| `0x00af` | mmRest | others | moderate | 2008, 2009, 2010, 2011 | 24 | `mus-04294cb8d3ff2871` (0:0xc136) |
| `0x00b0` | measSpec | others | strong | 2007, 2008, 2009, 2010, 2011 | 8, 26 | `mus-72605d9c006cbd00` (0:0x8274) |
| `0x00b1` | measExprAssign | others | weak | 2007, 2008, 2009, 2010, 2011 | 12, 24, 48, 72, 96, 120, 144, 168, 192, 216, 240, 264, 288, 312, 336, 384, 408, 456, 480, 504, 672, 744 | `mus-1ee495414f3aab74` (0:0x2bac) |
| `0x00b3` | namePosFull | others | weak | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x829c) |
| `0x00b4` | namePosFullStyle | others | moderate | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x82b6) |
| `0x00b5` | namePosFull | others | weak | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x831e) |
| `0x00b6` | namePosFullStyle | others | moderate | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x8338) |
| `0x00b7` | textBlock | others | moderate | 2007, 2008, 2009, 2010, 2011 | 36 | `mus-72605d9c006cbd00` (0:0x83a0) |
| `0x00b9` | drumLibName | others | weak | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x8cce) |
| `0x00ba` | textBlock | others | weak | 2007, 2008, 2009, 2010, 2011 | 12, 24 | `mus-72605d9c006cbd00` (0:0x8e6e) |
| `0x00bb` | pageSpec | others | strong | 2007, 2008, 2009, 2010, 2011 | 24 | `mus-72605d9c006cbd00` (0:0x9032) |
| `0x00be` | playDefs | others | weak | 2007, 2008, 2009, 2010, 2011 | 24 | `mus-72605d9c006cbd00` (0:0x9058) |
| `0x00bf` | playDumpShape | others | moderate | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-9a052a5a32d1fc9e` (0:0x75ca) |
| `0x00c0` | textExprDef | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x907e) |
| `0x00c2` | keysAttrib | others | weak | 2007, 2008, 2009, 2010, 2011 | 24, 48, 72, 96, 120, 144, 168, 192, 216, 240, 264, 288, 312, 336, 360, 384, 408, 432, 456, 480, 504, 528, 552, 576, 600, 624, 816, 1608 | `mus-72605d9c006cbd00` (0:0x9356) |
| `0x00cb` | repeatBack | others | strong | 2007, 2008, 2009, 2010, 2011 | 24 | `mus-060e10ac5f90cc2d` (0:0xee7a) |
| `0x00cc` | repeatPassList | others | moderate | 2007, 2008, 2009, 2010, 2011 | 24 | `mus-5fa3cc18b830282c` (0:0xc46c) |
| `0x00cd` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-f48a0bdf0b0df236` (0:0x9de2) |
| `0x00ce` | repeatPassList | others | moderate | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-5fa3cc18b830282c` (0:0xc4b8) |
| `0x00cf` | unknown | unknown/options | open | 2007, 2008 | 12, 24, 60 | `mus-34880307ea5e36e5` (0:0xa672) |
| `0x00d4` | separatesTextRepeat | others | weak | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-060e10ac5f90cc2d` (0:0xeea0) |
| `0x00d5` | shapeList | others | strong | 2007, 2008, 2009, 2010, 2011 | 60, 72, 84, 96, 108, 120, 132, 144, 156, 168, 180, 192, 204, 216, 228, 240, 252, 264, 276, 288, 300, 312, 324, 336, 360, 372, 384, 396, 408, 420, 432, 444, 456, 480, 492, 516, 528, 540, 552, 564, 576, 588, 612, 624, 648, 660, 684, 696, 708, 720, 744, 816, 840, 912, 984, 1008, 1056, 1176, 1188, 1320, 1368, 1452, 1464, 1476, 1536, 1560, 1692, 1812, 1884, 1980, 2076, 2148, 2160, 2292, 2460, 2484, 2520, 2688, 2784, 2976, 3552, 4248, 4344, 4488 | `mus-72605d9c006cbd00` (0:0x93ea) |
| `0x00d6` | shapeDef | others | strong | 2007, 2008, 2009, 2010, 2011 | 12, 24, 36, 60 | `mus-72605d9c006cbd00` (0:0xd49e) |
| `0x00d7` | shapeList | others | strong | 2007, 2008, 2009, 2010, 2011 | 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 156, 168, 180, 204, 216, 228, 240, 252, 276, 300, 324, 348, 372, 384, 420, 456, 468, 540, 564, 588, 600, 624, 660, 696, 720, 732, 828, 840, 924, 972, 1116, 1404, 1440 | `mus-72605d9c006cbd00` (0:0xdec8) |
| `0x00d8` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 36 | `mus-545b14c912206c49` (0:0x192f4) |
| `0x00d9` | smartShape | others | strong | 2007, 2008, 2009, 2010, 2011 | 96 | `mus-02eb545bb34c359c` (0:0x23836) |
| `0x00da` | smartShapeMeasMark | others | weak | 2007, 2008, 2009, 2010, 2011 | 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144, 168, 192, 228, 276, 1116 | `mus-02eb545bb34c359c` (0:0x247ae) |
| `0x00db` | drumLayoutPmmRef | others | weak | 2009, 2010, 2011 | 24 | `mus-6e2ae4fae31031c5` (0:0x1a0d4) |
| `0x00de` | ssLineStyle | others | strong | 2007, 2008, 2009, 2010, 2011 | 72 | `mus-72605d9c006cbd00` (0:0xf7d4) |
| `0x00df` | staffSystemSpec | others | strong | 2007, 2008, 2009, 2010, 2011 | 36 | `mus-72605d9c006cbd00` (0:0xfde0) |
| `0x00e1` | repeatStaffListScore | others | strong | 2007, 2008, 2009, 2010, 2011 | 12, 24, 36 | `mus-1ee495414f3aab74` (0:0x51b4) |
| `0x00e2` | repeatStaffListScore | others | strong | 2007, 2008, 2009, 2010, 2011 | 12, 24, 36 | `mus-1ee495414f3aab74` (0:0x51ce) |
| `0x00e4` | repeatStaffListScore | others | strong | 2007, 2008, 2009, 2010, 2011 | 12, 24, 36 | `mus-1ee495414f3aab74` (0:0x51e8) |
| `0x00e6` | staffPlayData | others | weak | 2007, 2008, 2009, 2010, 2011 | 48 | `mus-72605d9c006cbd00` (0:0xfe12) |
| `0x00e7` | staffSpec | others | weak | 2007, 2008, 2009, 2010, 2011 | 36, 72, 84 | `mus-72605d9c006cbd00` (0:0xfe50) |
| `0x00e8` | metaStaffStyle | others | moderate | 2007, 2008, 2009, 2010, 2011 | 132, 144 | `mus-72605d9c006cbd00` (0:0xfefc) |
| `0x00e9` | namePosFull | others | weak | 2007, 2008, 2009, 2010, 2011 | 24, 48, 72, 600, 2400 | `mus-c281f8a5e6e1fb32` (0:0x148c2) |
| `0x00eb` | shapeExprDef | others | moderate | 2007, 2008, 2009, 2010, 2011 | 36, 48, 60 | `mus-72605d9c006cbd00` (0:0x109d2) |
| `0x00ee` | timeUpper | others | strong | 2007, 2008, 2009, 2010, 2011 | 12, 24 | `mus-e217b8fd4aa4432d` (0:0x156c0) |
| `0x00f0` | tempoDef | others | strong | 2011 | 12 | `mus-676abda0af06c2ef` (0:0x24948) |
| `0x00f1` | textExprDef | others | strong | 2007, 2008, 2009, 2010, 2011 | 36, 48, 60, 72, 84, 96, 108, 120 | `mus-72605d9c006cbd00` (0:0x10dec) |
| `0x00f2` | textExpressionEnclosure | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x11364) |
| `0x00f3` | textRepeatAssign | others | moderate | 2007, 2008, 2009, 2010, 2011 | 24 | `mus-060e10ac5f90cc2d` (0:0x1a378) |
| `0x00f4` | textRepeatText | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x113cc) |
| `0x00f6` | textRepeatText | others | strong | 2007, 2008, 2009, 2010, 2011 | 12, 24, 36 | `mus-72605d9c006cbd00` (0:0x114d0) |
| `0x00f7` | keysAttrib | others | weak | 2007, 2008, 2009, 2010, 2011 | 48 | `mus-02eb545bb34c359c` (0:0x27d6c) |
| `0x0112` | viSetup | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x115e0) |
| `0x0113` | unknown | unknown/options | open | 2007, 2008, 2009, 2011 | 12, 468, 732, 948, 24264, 24276, 24456, 24624, 24672, 24696 | `mus-02eb545bb34c359c` (0:0x27e02) |
| `0x0114` | vstInfo | others | weak | 2007, 2008, 2009, 2010, 2011 | 516, 2280, 17232, 17388, 17400, 17484, 17700, 17712 | `mus-02eb545bb34c359c` (0:0x2dfc2) |
| `0x0115` | audioUnitInfo | others | strong | 2007, 2008, 2009, 2010, 2011 | 540 | `mus-02eb545bb34c359c` (0:0x32cb2) |
| `0x0116` | vstInfo | others | weak | 2007, 2008, 2009, 2010, 2011 | 540 | `mus-02eb545bb34c359c` (0:0x33106) |
| `0x011a` | partDef | others | strong | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x115fa) |
| `0x011b` | markingsDialogList | others | weak | 2007, 2008, 2009, 2010, 2011 | 8220, 8232, 8784, 8928, 8964, 9360 | `mus-72605d9c006cbd00` (0:0x11614) |
| `0x0120` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x1364a) |
| `0x0121` | unknown | unknown/options | open | 2008, 2009 | 12 | `mus-04294cb8d3ff2871` (0:0x23370) |
| `0x0122` | playDefs | others | weak | 2007, 2008, 2009, 2010, 2011 | 12 | `mus-72605d9c006cbd00` (0:0x13664) |
| `0x012c` | keysAttrib | others | weak | 2009, 2010, 2011 | 12, 108, 120, 132, 144, 156, 168, 180, 192, 204, 216, 228, 240, 252, 264, 276, 288, 300, 312, 324, 336, 348, 360, 372, 384, 396, 408, 420, 432, 444, 456, 468, 480, 492, 504, 516, 528, 540, 552, 564, 576, 588, 612, 624, 636, 672, 684, 708, 720, 744, 756, 768, 780, 792, 816, 828, 876 | `mus-6e2ae4fae31031c5` (0:0x2272e) |
| `0x012d` | markingsCategoryName | others | weak | 2009, 2010, 2011 | 36 | `mus-6e2ae4fae31031c5` (0:0x2285e) |
| `0x012e` | markingsCategoryName | others | weak | 2009, 2010, 2011 | 12, 24 | `mus-6e2ae4fae31031c5` (0:0x229bc) |
| `0x012f` | unknown | unknown/options | open | 2009, 2011 | 12, 24 | `mus-6e2ae4fae31031c5` (0:0x22aae) |
| `0x0130` | pageTextAssign | others | weak | 2009, 2010, 2011 | 12 | `mus-6e2ae4fae31031c5` (0:0x22b16) |
| `0x0132` | pageTextAssign | others | weak | 2009, 2010, 2011 | 12 | `mus-6e2ae4fae31031c5` (0:0x22b7e) |
| `0x0139` | channelPlayData | others | weak | 2010, 2011 | 12, 24, 48, 60, 72, 84, 96, 132, 144, 156, 180, 204, 228, 252, 264, 300, 336, 360, 564, 732 | `mus-ec5483b2ed383586` (0:0x13d88) |
| `0x013a` | percMapRef | others | strong | 2010, 2011 | 24, 36 | `mus-b85673dd398a956e` (0:0x137de) |
| `0x013b` | volumeValue | others | strong | 2010, 2011 | 12 | `mus-78835c0eb4eee5fc` (0:0x29e2c) |
| `0x013c` | bypassFxValue | others | strong | 2010, 2011 | 12 | `mus-78835c0eb4eee5fc` (0:0x29f16) |
| `0x013d` | percMapRef | others | strong | 2010, 2011 | 12 | `mus-b85673dd398a956e` (0:0x1396e) |
| `0x013e` | percMapRef | others | strong | 2010, 2011 | 24 | `mus-b85673dd398a956e` (0:0x13a3e) |
| `0x0141` | drumLayoutPmmRef | others | moderate | 2010, 2011 | 12 | `mus-ec5483b2ed383586` (0:0x13e9e) |
| `0x03ef` | acciAlter | details | weak | 2007, 2008, 2009, 2010, 2011 | 20, 40 | `mus-f4900d133f46cdec` (1:0x0) |
| `0x03f0` | entrySize | details | weak | 2011 | 10 | `mus-7d792c9572f27bf1` (1:0x0) |
| `0x03f1` | articAssign | details | weak | 2007, 2008, 2009, 2010, 2011 | 20, 40 | `mus-f4900d133f46cdec` (1:0x48) |
| `0x03f2` | baselinesChords | details | strong | 2007, 2008, 2009, 2010, 2011 | 10 | `mus-9a052a5a32d1fc9e` (1:0x0) |
| `0x03f3` | baselinesExprAboveStaff | details | weak | 2007, 2008, 2009, 2010, 2011 | 10 | `mus-9a052a5a32d1fc9e` (1:0x34) |
| `0x03f4` | baselinesExprBelowStaff | details | strong | 2007, 2008, 2009, 2010, 2011 | 10 | `mus-9a052a5a32d1fc9e` (1:0x4e) |
| `0x03f5` | baselinesFingerboards | details | strong | 2007, 2008, 2009, 2010, 2011 | 10 | `mus-65a7d5336541c8f1` (1:0x4e) |
| `0x03f6` | baselinesExprAboveStaff | details | weak | 2007, 2008, 2009, 2010, 2011 | 100 | `mus-9a052a5a32d1fc9e` (1:0x68) |
| `0x03f7` | baselinesExprAboveStaff | details | weak | 2007, 2008, 2009, 2010, 2011 | 100 | `mus-9a052a5a32d1fc9e` (1:0xdc) |
| `0x03f8` | baselinesExprAboveStaff | details | weak | 2007, 2008, 2009, 2010, 2011 | 10, 100 | `mus-9a052a5a32d1fc9e` (1:0x150) |
| `0x03fd` | beamExtendDownStem | details | weak | 2007, 2010, 2011 | 10 | `mus-46b91a9536cf1b3b` (1:0x1aa) |
| `0x0401` | beamAltPrimDownStem | details | moderate | 2007, 2008, 2009, 2010, 2011 | 20 | `mus-f4900d133f46cdec` (1:0x2ee) |
| `0x0402` | beamAltPrimUpStem | details | weak | 2007, 2008, 2009, 2010, 2011 | 20 | `mus-f4900d133f46cdec` (1:0x106e) |
| `0x0403` | beamAltSecDownStem | details | weak | 2011 | 20 | `mus-868209cf352f2a76` (1:0x2264) |
| `0x0404` | beamAltSecUpStem | details | weak | 2011 | 20 | `mus-868209cf352f2a76` (1:0x233c) |
| `0x0406` | centerShape | details | strong | 2007, 2008, 2009, 2010, 2011 | 30 | `mus-890653fa5ea58b74` (1:0x133e) |
| `0x0407` | unknown | unknown/options | open | 2007, 2008, 2009 | 20 | `mus-939fabfa49231af2` (1:0x5fc) |
| `0x040a` | crossChord | details | weak | 2007, 2008, 2009, 2010, 2011 | 160 | `mus-9a052a5a32d1fc9e` (1:0x1c4) |
| `0x040c` | crossStaff | details | weak | 2007, 2008, 2009, 2010, 2011 | 10, 20, 30 | `mus-f4900d133f46cdec` (1:0x1bd2) |
| `0x040d` | dotOffset | details | weak | 2008, 2009, 2010, 2011 | 10 | `mus-027e3cdd17c8b370` (1:0x3a70) |
| `0x040e` | unknown | unknown/options | open | 2007, 2008, 2009 | 10 | `mus-9a052a5a32d1fc9e` (1:0xa04) |
| `0x040f` | unknown | unknown/options | open | 2007, 2008, 2009 | 10, 20, 30, 40, 50 | `mus-9a052a5a32d1fc9e` (1:0x4b04) |
| `0x0410` | staffSize | details | strong | 2008, 2009, 2010, 2011 | 10 | `mus-027e3cdd17c8b370` (1:0x87d2) |
| `0x0411` | unknown | unknown/options | open | 2008 | 20 | `mus-fda3226056415703` (1:0x1d8) |
| `0x0413` | fretboard | details | strong | 2007, 2008, 2009, 2010, 2011 | 10, 30, 40, 50 | `mus-f4900d133f46cdec` (1:0xd816) |
| `0x0414` | gfhold | details | weak | 2007, 2008, 2009, 2010, 2011 | 20 | `mus-9a052a5a32d1fc9e` (1:0x574c) |
| `0x041a` | unknown | unknown/options | open | 2007, 2008, 2009, 2010 | 10, 20 | `mus-31f3ebb3a171fb74` (1:0x269c6) |
| `0x041b` | lyricEntryInfo | details | moderate | 2010, 2011 | 10 | `mus-edc656eb52c10f40` (1:0x1c1f4) |
| `0x041e` | midiExprs | details | weak | 2007, 2008, 2009, 2010, 2011 | 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 120, 150, 180 | `mus-31f3ebb3a171fb74` (1:0x27d12) |
| `0x041f` | measNumbIndivPos | details | strong | 2007, 2008, 2009, 2010, 2011 | 30 | `mus-939fabfa49231af2` (1:0x23f26) |
| `0x0420` | unknown | unknown/options | open | 2007, 2008, 2009, 2010, 2011 | 10, 20 | `mus-31f3ebb3a171fb74` (1:0x27ece) |
| `0x0421` | staffGroup | details | weak | 2007, 2008, 2009, 2010, 2011 | 40 | `mus-f4900d133f46cdec` (1:0x28d7e) |
| `0x0422` | noteAlter | details | strong | 2009, 2010, 2011 | 20, 40 | `mus-537315e806efc94a` (1:0x1d922) |
| `0x0423` | unknown | unknown/options | open | 2007, 2008 | 10, 20, 30 | `mus-890653fa5ea58b74` (1:0x28944) |
| `0x0424` | perfData | details | weak | 2007, 2008, 2009, 2010, 2011 | 10, 20, 30, 40, 50, 60, 70, 80, 90 | `mus-939fabfa49231af2` (1:0x24212) |
| `0x0425` | unknown | unknown/options | open | 2007, 2008 | 10 | `mus-31f3ebb3a171fb74` (1:0x29cfe) |
| `0x0426` | shapeNote | details | weak | 2007, 2008, 2009, 2010, 2011 | 10, 210 | `mus-939fabfa49231af2` (1:0x2817c) |
| `0x0427` | shapeNoteStyle | details | weak | 2007, 2008, 2009, 2010, 2011 | 10, 210 | `mus-a021ae5c2bc8fb35` (1:0x5770) |
| `0x0428` | smartShapeEntryMark | details | weak | 2007, 2008, 2009, 2010, 2011 | 10, 20, 30, 40, 50, 60, 70 | `mus-f4900d133f46cdec` (1:0x293de) |
| `0x0429` | unknown | unknown/options | open | 2007, 2008, 2009, 2010 | 10, 20, 30, 40, 50, 60 | `mus-31f3ebb3a171fb74` (1:0x2a0ba) |
| `0x042a` | stemAdjust | details | weak | 2007, 2008, 2009, 2010, 2011 | 10, 40 | `mus-f4900d133f46cdec` (1:0x29412) |
| `0x042c` | unknown | unknown/options | open | 2009, 2010, 2011 | 10 | `mus-537315e806efc94a` (1:0x1f242) |
| `0x042d` | unknown | unknown/options | open | 2008, 2009, 2010, 2011 | 10 | `mus-41fda45672367dcb` (1:0x5bcc) |
| `0x042e` | tieAlterEnd | details | weak | 2007, 2008, 2009, 2010, 2011 | 30 | `mus-f4900d133f46cdec` (1:0x29bb0) |
| `0x042f` | tieAlterStart | details | moderate | 2007, 2008, 2009, 2010, 2011 | 30, 60, 90, 120 | `mus-f4900d133f46cdec` (1:0x29cc4) |
| `0x0430` | tupletDef | details | weak | 2007, 2008, 2009, 2010, 2011 | 30 | `mus-890653fa5ea58b74` (1:0x29c02) |
| `0x0443` | baselinesSysChords | details | strong | 2007, 2008, 2009, 2010, 2011 | 10 | `mus-9a052a5a32d1fc9e` (1:0x5770) |
| `0x0445` | unknown | unknown/options | open | 2007, 2008, 2011 | 10 | `mus-31f3ebb3a171fb74` (1:0x2ba88) |
| `0x0447` | unknown | unknown/options | open | 2011 | 10 | `mus-acc32f8474baffa8` (1:0x18562) |
| `0x0449` | baselinesSysLyricsVerse | details | weak | 2007, 2008, 2009, 2010, 2011 | 10, 20 | `mus-31f3ebb3a171fb74` (1:0x2baa2) |
| `0x0450` | chordAssign | details | weak | 2010, 2011 | 30, 60 | `mus-67f4bfd03dde3846` (1:0x1cf3e) |
| `0x0451` | percussionNoteCode | details | weak | 2010, 2011 | 10, 20, 30 | `mus-b85673dd398a956e` (1:0x1bb5a) |
| `0x0452` | lyrDataChorus | details | weak | 2011 | 20 | `mus-183512503c0c7345` (1:0x190d6) |
| `0x0454` | lyrDataVerse | details | weak | 2011 | 20, 40, 60 | `mus-d9abedbf7916a9dd` (1:0x1846c) |
| `0x0455` | activeLyric | details | strong | 2011 | 10 | `mus-5626988fe8cc0d48` (1:0x16aae) |
