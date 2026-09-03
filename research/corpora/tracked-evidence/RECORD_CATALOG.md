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
| `0x000f` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x0) |
| `0x0010` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x1a) |
| `0x0011` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x34) |
| `0x0012` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x4e) |
| `0x0013` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x68) |
| `0x0014` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x82) |
| `0x0015` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x9c) |
| `0x0016` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xb6) |
| `0x0017` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xd0) |
| `0x0018` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xea) |
| `0x0019` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x104) |
| `0x001a` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x11e) |
| `0x001b` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x138) |
| `0x001c` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x152) |
| `0x001d` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x16c) |
| `0x001e` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x186) |
| `0x001f` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x1a0) |
| `0x0020` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 36 | `mus-86fafceb1ef2ebe2` (0:0x1ba) |
| `0x0021` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x1ec) |
| `0x0022` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x206) |
| `0x0023` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x220) |
| `0x0024` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x23a) |
| `0x0025` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x254) |
| `0x0026` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 276 | `mus-86fafceb1ef2ebe2` (0:0x26e) |
| `0x0027` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 24 | `mus-86fafceb1ef2ebe2` (0:0x390) |
| `0x0028` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 180 | `mus-86fafceb1ef2ebe2` (0:0x3b6) |
| `0x0029` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x478) |
| `0x002a` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x492) |
| `0x002b` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x4ac) |
| `0x002c` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x4c6) |
| `0x002d` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x4e0) |
| `0x002e` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x4fa) |
| `0x002f` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x514) |
| `0x0030` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x52e) |
| `0x0031` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x548) |
| `0x0032` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x562) |
| `0x0033` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x57c) |
| `0x0034` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x596) |
| `0x0035` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x5b0) |
| `0x0036` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 1536, 1800 | `mus-86fafceb1ef2ebe2` (0:0x5ca) |
| `0x0037` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 36 | `mus-86fafceb1ef2ebe2` (0:0xbd8) |
| `0x0038` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xc0a) |
| `0x0039` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xc24) |
| `0x003a` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xc3e) |
| `0x003b` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xc58) |
| `0x003c` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xc72) |
| `0x003d` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xc8c) |
| `0x003e` | unknown | unknown/options | open | 2008, 2011, 2012 | 84 | `mus-ed8096788fa377bf` (0:0xca6) |
| `0x0040` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xca6) |
| `0x0041` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xcc0) |
| `0x0042` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 24 | `mus-86fafceb1ef2ebe2` (0:0xcda) |
| `0x0043` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 24 | `mus-86fafceb1ef2ebe2` (0:0xd00) |
| `0x0045` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 60 | `mus-86fafceb1ef2ebe2` (0:0xd26) |
| `0x0046` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 36 | `mus-86fafceb1ef2ebe2` (0:0xd70) |
| `0x0047` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12, 24 | `mus-86fafceb1ef2ebe2` (0:0xda2) |
| `0x0048` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12, 24 | `mus-86fafceb1ef2ebe2` (0:0xdbc) |
| `0x0049` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xdd6) |
| `0x004a` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xdf0) |
| `0x004b` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xe0a) |
| `0x004c` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xe24) |
| `0x004d` | unknown | unknown/options | open | 2007, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xe3e) |
| `0x004e` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xe58) |
| `0x004f` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xe72) |
| `0x0050` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xe8c) |
| `0x0051` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xea6) |
| `0x0052` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xec0) |
| `0x0053` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xeda) |
| `0x0054` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xef4) |
| `0x0055` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xf0e) |
| `0x0056` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xf28) |
| `0x0057` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xf42) |
| `0x0058` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xf5c) |
| `0x0059` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12, 276 | `mus-86fafceb1ef2ebe2` (0:0xf76) |
| `0x005a` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0xf90) |
| `0x005b` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 108 | `mus-86fafceb1ef2ebe2` (0:0xfaa) |
| `0x005c` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 96, 108 | `mus-86fafceb1ef2ebe2` (0:0x1024) |
| `0x005d` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x1092) |
| `0x005e` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x10ac) |
| `0x005f` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x10c6) |
| `0x0060` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x10e0) |
| `0x0061` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x10fa) |
| `0x0062` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 48 | `mus-86fafceb1ef2ebe2` (0:0x1114) |
| `0x0063` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 48 | `mus-86fafceb1ef2ebe2` (0:0x1152) |
| `0x0064` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 60 | `mus-86fafceb1ef2ebe2` (0:0x1190) |
| `0x0065` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 24 | `mus-86fafceb1ef2ebe2` (0:0x11da) |
| `0x0066` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 120 | `mus-86fafceb1ef2ebe2` (0:0x1200) |
| `0x0068` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 108 | `mus-86fafceb1ef2ebe2` (0:0x1286) |
| `0x0069` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x1300) |
| `0x006a` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x131a) |
| `0x006b` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x1334) |
| `0x006c` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 36 | `mus-86fafceb1ef2ebe2` (0:0x134e) |
| `0x006d` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 324, 360 | `mus-86fafceb1ef2ebe2` (0:0x1380) |
| `0x006f` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 24, 36 | `mus-86fafceb1ef2ebe2` (0:0x14d2) |
| `0x0070` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 48 | `mus-86fafceb1ef2ebe2` (0:0x14f8) |
| `0x0071` | unknown | unknown/options | open | 2008, 2011, 2012 | 84 | `mus-ed8096788fa377bf` (0:0x158a) |
| `0x0079` | unknown | unknown/options | open | 2012 | 60 | `mus-4ffcb6b07d802e7c` (0:0x1838) |
| `0x007a` | beatChart | others | weak | 2012 | 48, 84, 108, 144 | `mus-aaff1568ec2bb9ef` (0:0x1852) |
| `0x007b` | unknown | unknown/options | open | 2012 | 24 | `mus-f99c887dff9cdaa1` (0:0x1852) |
| `0x0086` | staffPlayData | others | weak | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x1536) |
| `0x0089` | unknown | unknown/options | open | 2012 | 396, 408, 420, 432 | `mus-80f8c631de435726` (0:0x1852) |
| `0x008a` | unknown | unknown/options | open | 2012 | 12 | `mus-80f8c631de435726` (0:0x225a) |
| `0x008b` | unknown | unknown/options | open | 2012 | 24 | `mus-80f8c631de435726` (0:0x22f6) |
| `0x008d` | layerAtts | others | weak | 2007, 2008, 2011, 2012 | 12, 36 | `mus-86fafceb1ef2ebe2` (0:0x1550) |
| `0x0090` | fontName | others | weak | 2007, 2008, 2011, 2012 | 24, 36 | `mus-86fafceb1ef2ebe2` (0:0x15d0) |
| `0x0092` | frameSpec | others | strong | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x1732) |
| `0x0095` | staffPlayData | others | weak | 2007, 2008, 2011, 2012 | 72 | `mus-86fafceb1ef2ebe2` (0:0x179a) |
| `0x0097` | staffPlayData | others | weak | 2007, 2008, 2011, 2012 | 156 | `mus-86fafceb1ef2ebe2` (0:0x17f0) |
| `0x009e` | hpOptions | others | weak | 2012 | 36 | `mus-f99c887dff9cdaa1` (0:0x1c30) |
| `0x009f` | shapeList | others | weak | 2007, 2008, 2011, 2012 | 24, 48 | `mus-86fafceb1ef2ebe2` (0:0x189a) |
| `0x00a3` | layerAtts | others | weak | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x18fe) |
| `0x00a4` | unknown | unknown/options | open | 2012 | 360 | `mus-f95d2ea21b99022e` (0:0x2220) |
| `0x00a5` | unknown | unknown/options | open | 2012 | 12 | `mus-4ffcb6b07d802e7c` (0:0x1e98) |
| `0x00a6` | metaKeySig | others | weak | 2012 | 24 | `mus-f99c887dff9cdaa1` (0:0x1d54) |
| `0x00a7` | metaClef | others | strong | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x1966) |
| `0x00a8` | unknown | unknown/options | open | 2012 | 12 | `mus-80f8c631de435726` (0:0x2c3e) |
| `0x00a9` | metaKeySig | others | weak | 2012 | 12 | `mus-f99c887dff9cdaa1` (0:0x204a) |
| `0x00b0` | measSpec | others | strong | 2007, 2008, 2011, 2012 | 8, 26 | `mus-86fafceb1ef2ebe2` (0:0x1b3a) |
| `0x00b1` | unknown | unknown/options | open | 2012 | 24 | `mus-80f8c631de435726` (0:0x2e10) |
| `0x00b7` | textBlock | others | strong | 2007, 2008, 2011, 2012 | 36 | `mus-86fafceb1ef2ebe2` (0:0x1b8a) |
| `0x00b9` | staffPlayData | others | weak | 2007, 2008, 2011, 2012 | 12, 24 | `mus-86fafceb1ef2ebe2` (0:0x1bbc) |
| `0x00ba` | staffPlayData | others | weak | 2007, 2008, 2011, 2012 | 24, 36, 48 | `mus-86fafceb1ef2ebe2` (0:0x1bd6) |
| `0x00bb` | partGlobals | others | strong | 2007, 2008, 2011, 2012 | 24 | `mus-86fafceb1ef2ebe2` (0:0x1bfc) |
| `0x00bc` | unknown | unknown/options | open | 2012 | 36 | `mus-80f8c631de435726` (0:0x2ee6) |
| `0x00be` | staffPlayData | others | weak | 2007, 2008, 2011, 2012 | 24 | `mus-86fafceb1ef2ebe2` (0:0x1c22) |
| `0x00bf` | unknown | unknown/options | open | 2012 | 12 | `mus-80f8c631de435726` (0:0x2f3e) |
| `0x00c0` | unknown | unknown/options | open | 2011, 2012 | 12 | `mus-d89543077eefeae6` (0:0x1e24) |
| `0x00c2` | unknown | unknown/options | open | 2008 | 144 | `mus-ed8096788fa377bf` (0:0x1db4) |
| `0x00d5` | shapeList | others | strong | 2007, 2008, 2011, 2012 | 84, 168, 180, 264 | `mus-86fafceb1ef2ebe2` (0:0x1c48) |
| `0x00d6` | shapeDef | others | strong | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x1ee2) |
| `0x00d7` | shapeList | others | strong | 2007, 2008, 2011, 2012 | 36, 48, 60, 84 | `mus-86fafceb1ef2ebe2` (0:0x1f30) |
| `0x00d8` | unknown | unknown/options | open | 2012 | 36 | `mus-80f8c631de435726` (0:0x3444) |
| `0x00d9` | unknown | unknown/options | open | 2007 | 96 | `mus-86fafceb1ef2ebe2` (0:0x2026) |
| `0x00da` | unknown | unknown/options | open | 2007 | 12, 24 | `mus-86fafceb1ef2ebe2` (0:0x2102) |
| `0x00db` | hpOptions | others | weak | 2012 | 24 | `mus-f99c887dff9cdaa1` (0:0x262a) |
| `0x00de` | ssLineStyle | others | strong | 2007, 2008, 2011, 2012 | 72 | `mus-86fafceb1ef2ebe2` (0:0x2142) |
| `0x00df` | staffSystemSpec | others | strong | 2007, 2008, 2011, 2012 | 36 | `mus-86fafceb1ef2ebe2` (0:0x2244) |
| `0x00e1` | unknown | unknown/options | open | 2012 | 12 | `mus-42c6d5caf1e3b6ff` (0:0x2550) |
| `0x00e2` | unknown | unknown/options | open | 2012 | 12 | `mus-42c6d5caf1e3b6ff` (0:0x259e) |
| `0x00e4` | unknown | unknown/options | open | 2012 | 12 | `mus-42c6d5caf1e3b6ff` (0:0x25ec) |
| `0x00e5` | unknown | unknown/options | open | 2012 | 12 | `mus-42c6d5caf1e3b6ff` (0:0x263a) |
| `0x00e6` | staffPlayData | others | weak | 2007, 2008, 2011, 2012 | 48 | `mus-86fafceb1ef2ebe2` (0:0x2276) |
| `0x00e7` | staffSpec | others | weak | 2007, 2008, 2011, 2012 | 72, 84, 96 | `mus-86fafceb1ef2ebe2` (0:0x22b4) |
| `0x00e8` | staffStyle | others | weak | 2012 | 300 | `mus-f99c887dff9cdaa1` (0:0x289e) |
| `0x00eb` | unknown | unknown/options | open | 2012 | 36 | `mus-80f8c631de435726` (0:0x37be) |
| `0x00f1` | unknown | unknown/options | open | 2011, 2012 | 36, 48 | `mus-d89543077eefeae6` (0:0x24a0) |
| `0x00f7` | unknown | unknown/options | open | 2012 | 48 | `mus-4ffcb6b07d802e7c` (0:0x28dc) |
| `0x0112` | unknown | unknown/options | open | 2007, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x2360) |
| `0x011a` | partGlobals | others | strong | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x237a) |
| `0x0120` | unknown | unknown/options | open | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x2394) |
| `0x0122` | moviePlayerState | others | strong | 2007, 2008, 2011, 2012 | 12 | `mus-86fafceb1ef2ebe2` (0:0x23ae) |
| `0x012c` | markingsDialogList | others | weak | 2011, 2012 | 12, 24 | `mus-d89543077eefeae6` (0:0x25e6) |
| `0x012d` | markingsCategoryName | others | weak | 2011, 2012 | 36 | `mus-eb742704c65232a2` (0:0x23be) |
| `0x012e` | markingsCategoryName | others | weak | 2011, 2012 | 12, 24, 36 | `mus-eb742704c65232a2` (0:0x251c) |
| `0x0130` | categoryStaffListScore | others | weak | 2011, 2012 | 12 | `mus-eb742704c65232a2` (0:0x260e) |
| `0x0132` | categoryStaffListScore | others | weak | 2011, 2012 | 12 | `mus-eb742704c65232a2` (0:0x26de) |
| `0x013b` | unknown | unknown/options | open | 2012 | 12 | `mus-f95d2ea21b99022e` (0:0x355a) |
| `0x013c` | unknown | unknown/options | open | 2012 | 12 | `mus-f95d2ea21b99022e` (0:0x3644) |
| `0x03f1` | articAssign | details | weak | 2012 | 20 | `mus-4a7e05d0278fdb3a` (1:0x0) |
| `0x03f2` | baselinesExprBelowStaff | details | weak | 2008, 2011, 2012 | 10 | `mus-23a91fcc4d98768c` (1:0x0) |
| `0x03f3` | baselinesExprBelowStaff | details | weak | 2008, 2011, 2012 | 10 | `mus-23a91fcc4d98768c` (1:0x1a) |
| `0x03f4` | baselinesExprBelowStaff | details | weak | 2008, 2011, 2012 | 10 | `mus-23a91fcc4d98768c` (1:0x34) |
| `0x03f6` | baselinesExprBelowStaff | details | weak | 2008, 2011, 2012 | 100 | `mus-23a91fcc4d98768c` (1:0x4e) |
| `0x03f7` | baselinesExprBelowStaff | details | weak | 2008, 2011, 2012 | 100 | `mus-23a91fcc4d98768c` (1:0xc2) |
| `0x03f8` | baselinesExprBelowStaff | details | weak | 2008, 2011, 2012 | 100 | `mus-23a91fcc4d98768c` (1:0x136) |
| `0x0402` | unknown | unknown/options | open | 2012 | 20 | `mus-f95d2ea21b99022e` (1:0x1aa) |
| `0x0414` | gfhold | details | strong | 2008, 2011, 2012 | 20 | `mus-23a91fcc4d98768c` (1:0x1aa) |
| `0x041d` | unknown | unknown/options | open | 2012 | 40 | `mus-80f8c631de435726` (1:0x336) |
| `0x0426` | unknown | unknown/options | open | 2012 | 10, 410 | `mus-f361e13d1cc39a3e` (1:0x1ce) |
| `0x0455` | activeLyric | details | weak | 2011, 2012 | 10 | `mus-eb742704c65232a2` (1:0x1ce) |
