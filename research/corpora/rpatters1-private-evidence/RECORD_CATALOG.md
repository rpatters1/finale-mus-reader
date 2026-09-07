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
| `0x000f` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x0) |
| `0x0010` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1a) |
| `0x0011` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x34) |
| `0x0012` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x4e) |
| `0x0013` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x68) |
| `0x0014` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x82) |
| `0x0015` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x9c) |
| `0x0016` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xb6) |
| `0x0017` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xd0) |
| `0x0018` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xea) |
| `0x0019` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x104) |
| `0x001a` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x11e) |
| `0x001b` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x138) |
| `0x001c` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x152) |
| `0x001d` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x16c) |
| `0x001e` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x186) |
| `0x001f` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1a0) |
| `0x0020` | unknown | unknown/options | open | 2012 | 36 | `mus-c53758b97dfa5f7e` (0:0x1ba) |
| `0x0021` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1ec) |
| `0x0022` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x206) |
| `0x0023` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x220) |
| `0x0024` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x23a) |
| `0x0025` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x254) |
| `0x0026` | unknown | unknown/options | open | 2012 | 276 | `mus-c53758b97dfa5f7e` (0:0x26e) |
| `0x0027` | unknown | unknown/options | open | 2012 | 24 | `mus-c53758b97dfa5f7e` (0:0x390) |
| `0x0028` | unknown | unknown/options | open | 2012 | 180 | `mus-c53758b97dfa5f7e` (0:0x3b6) |
| `0x0029` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x478) |
| `0x002a` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x492) |
| `0x002b` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x4ac) |
| `0x002c` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x4c6) |
| `0x002d` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x4e0) |
| `0x002e` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x4fa) |
| `0x002f` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x514) |
| `0x0030` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x52e) |
| `0x0031` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x548) |
| `0x0032` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x562) |
| `0x0033` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x57c) |
| `0x0034` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x596) |
| `0x0035` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x5b0) |
| `0x0036` | unknown | unknown/options | open | 2012 | 1800 | `mus-c53758b97dfa5f7e` (0:0x5ca) |
| `0x0037` | unknown | unknown/options | open | 2012 | 36 | `mus-c53758b97dfa5f7e` (0:0xce0) |
| `0x0038` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xd12) |
| `0x0039` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xd2c) |
| `0x003a` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xd46) |
| `0x003b` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xd60) |
| `0x003c` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xd7a) |
| `0x003d` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xd94) |
| `0x003e` | unknown | unknown/options | open | 2012 | 84 | `mus-c53758b97dfa5f7e` (0:0xdae) |
| `0x0040` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xe10) |
| `0x0041` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xe2a) |
| `0x0042` | unknown | unknown/options | open | 2012 | 24 | `mus-c53758b97dfa5f7e` (0:0xe44) |
| `0x0043` | unknown | unknown/options | open | 2012 | 24 | `mus-c53758b97dfa5f7e` (0:0xe6a) |
| `0x0045` | unknown | unknown/options | open | 2012 | 60 | `mus-c53758b97dfa5f7e` (0:0xe90) |
| `0x0046` | unknown | unknown/options | open | 2012 | 36 | `mus-c53758b97dfa5f7e` (0:0xeda) |
| `0x0047` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xf0c) |
| `0x0048` | unknown | unknown/options | open | 2012 | 24 | `mus-c53758b97dfa5f7e` (0:0xf26) |
| `0x0049` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xf4c) |
| `0x004a` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xf66) |
| `0x004b` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xf80) |
| `0x004c` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xf9a) |
| `0x004e` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xfb4) |
| `0x004f` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xfce) |
| `0x0050` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0xfe8) |
| `0x0051` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1002) |
| `0x0052` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x101c) |
| `0x0053` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1036) |
| `0x0054` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1050) |
| `0x0055` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x106a) |
| `0x0056` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1084) |
| `0x0057` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x109e) |
| `0x0058` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x10b8) |
| `0x0059` | unknown | unknown/options | open | 2012 | 276 | `mus-c53758b97dfa5f7e` (0:0x10d2) |
| `0x005a` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x11f4) |
| `0x005b` | unknown | unknown/options | open | 2012 | 108 | `mus-c53758b97dfa5f7e` (0:0x120e) |
| `0x005c` | unknown | unknown/options | open | 2012 | 108 | `mus-c53758b97dfa5f7e` (0:0x1288) |
| `0x005d` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1302) |
| `0x005e` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x131c) |
| `0x005f` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1336) |
| `0x0060` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1350) |
| `0x0061` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x136a) |
| `0x0062` | unknown | unknown/options | open | 2012 | 48 | `mus-c53758b97dfa5f7e` (0:0x1384) |
| `0x0063` | unknown | unknown/options | open | 2012 | 48 | `mus-c53758b97dfa5f7e` (0:0x13c2) |
| `0x0064` | unknown | unknown/options | open | 2012 | 60 | `mus-c53758b97dfa5f7e` (0:0x1400) |
| `0x0065` | unknown | unknown/options | open | 2012 | 24 | `mus-c53758b97dfa5f7e` (0:0x144a) |
| `0x0066` | unknown | unknown/options | open | 2012 | 120 | `mus-c53758b97dfa5f7e` (0:0x1470) |
| `0x0068` | unknown | unknown/options | open | 2012 | 108 | `mus-c53758b97dfa5f7e` (0:0x14f6) |
| `0x0069` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1570) |
| `0x006a` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x158a) |
| `0x006b` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x15a4) |
| `0x006c` | unknown | unknown/options | open | 2012 | 36 | `mus-c53758b97dfa5f7e` (0:0x15be) |
| `0x006d` | unknown | unknown/options | open | 2012 | 360 | `mus-c53758b97dfa5f7e` (0:0x15f0) |
| `0x006f` | unknown | unknown/options | open | 2012 | 36 | `mus-c53758b97dfa5f7e` (0:0x1766) |
| `0x0070` | unknown | unknown/options | open | 2012 | 48 | `mus-c53758b97dfa5f7e` (0:0x1798) |
| `0x0071` | unknown | unknown/options | open | 2012 | 84 | `mus-c53758b97dfa5f7e` (0:0x17d6) |
| `0x0086` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1838) |
| `0x0089` | unknown | unknown/options | open | 2012 | 600 | `mus-c53758b97dfa5f7e` (0:0x1852) |
| `0x008a` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1ab8) |
| `0x008b` | unknown | unknown/options | open | 2012 | 24 | `mus-c53758b97dfa5f7e` (0:0x1ad2) |
| `0x008d` | unknown | unknown/options | open | 2012 | 12, 36 | `mus-c53758b97dfa5f7e` (0:0x1af8) |
| `0x0090` | unknown | unknown/options | open | 2012 | 24, 36 | `mus-c53758b97dfa5f7e` (0:0x1b78) |
| `0x0092` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1e06) |
| `0x0095` | unknown | unknown/options | open | 2012 | 72 | `mus-c53758b97dfa5f7e` (0:0x1e20) |
| `0x0097` | unknown | unknown/options | open | 2012 | 156 | `mus-c53758b97dfa5f7e` (0:0x1e76) |
| `0x009f` | unknown | unknown/options | open | 2012 | 24, 48 | `mus-c53758b97dfa5f7e` (0:0x1f20) |
| `0x00a3` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x1faa) |
| `0x00a7` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x2012) |
| `0x00b0` | unknown | unknown/options | open | 2012 | 26 | `mus-c53758b97dfa5f7e` (0:0x21e6) |
| `0x00b7` | unknown | unknown/options | open | 2012 | 36 | `mus-c53758b97dfa5f7e` (0:0x220e) |
| `0x00b9` | unknown | unknown/options | open | 2012 | 24 | `mus-c53758b97dfa5f7e` (0:0x2240) |
| `0x00ba` | unknown | unknown/options | open | 2012 | 36 | `mus-c53758b97dfa5f7e` (0:0x2266) |
| `0x00bb` | unknown | unknown/options | open | 2012 | 24 | `mus-c53758b97dfa5f7e` (0:0x2298) |
| `0x00bc` | unknown | unknown/options | open | 2012 | 36 | `mus-c53758b97dfa5f7e` (0:0x22be) |
| `0x00be` | unknown | unknown/options | open | 2012 | 24 | `mus-c53758b97dfa5f7e` (0:0x22f0) |
| `0x00d5` | unknown | unknown/options | open | 2012 | 180, 264 | `mus-c53758b97dfa5f7e` (0:0x2316) |
| `0x00d6` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x25b0) |
| `0x00d7` | unknown | unknown/options | open | 2012 | 60, 84 | `mus-c53758b97dfa5f7e` (0:0x25fe) |
| `0x00de` | unknown | unknown/options | open | 2012 | 72 | `mus-c53758b97dfa5f7e` (0:0x26f4) |
| `0x00df` | unknown | unknown/options | open | 2012 | 36 | `mus-c53758b97dfa5f7e` (0:0x27f6) |
| `0x00e6` | unknown | unknown/options | open | 2012 | 48 | `mus-c53758b97dfa5f7e` (0:0x2828) |
| `0x00e7` | unknown | unknown/options | open | 2012 | 96 | `mus-c53758b97dfa5f7e` (0:0x2866) |
| `0x011a` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x2942) |
| `0x0120` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x295c) |
| `0x0122` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x2976) |
| `0x012d` | unknown | unknown/options | open | 2012 | 36 | `mus-c53758b97dfa5f7e` (0:0x2990) |
| `0x012e` | unknown | unknown/options | open | 2012 | 24, 36 | `mus-c53758b97dfa5f7e` (0:0x2aee) |
| `0x0130` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x2c34) |
| `0x0132` | unknown | unknown/options | open | 2012 | 12 | `mus-c53758b97dfa5f7e` (0:0x2d04) |
| `0x03f2` | unknown | unknown/options | open | 2012 | 10 | `mus-c53758b97dfa5f7e` (1:0x0) |
| `0x03f3` | unknown | unknown/options | open | 2012 | 10 | `mus-c53758b97dfa5f7e` (1:0x1a) |
| `0x03f4` | unknown | unknown/options | open | 2012 | 10 | `mus-c53758b97dfa5f7e` (1:0x34) |
| `0x03f6` | unknown | unknown/options | open | 2012 | 100 | `mus-c53758b97dfa5f7e` (1:0x4e) |
| `0x03f7` | unknown | unknown/options | open | 2012 | 100 | `mus-c53758b97dfa5f7e` (1:0xc2) |
| `0x03f8` | unknown | unknown/options | open | 2012 | 100 | `mus-c53758b97dfa5f7e` (1:0x136) |
| `0x0414` | unknown | unknown/options | open | 2012 | 20 | `mus-c53758b97dfa5f7e` (1:0x1aa) |
