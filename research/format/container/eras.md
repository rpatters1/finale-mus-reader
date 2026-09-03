# Format eras

**Covers:** The four broad MUS epochs, platform coverage risk, and the checksum/compression/wrapping summary per era.
**Read when:** Orienting on which epoch a problem belongs to, or choosing an epoch gate.
**Confidence:** confirmed for era boundaries at the container level.

## Proposed format eras

| Era | Products in corpus | Structural evidence | Status |
|---|---|---|---|
| Pre-banner | apparent Finale 2 | Distinct header/body; no `ENIGMA BINARY FILE` | Separate parser likely |
| Uncompressed fixed-row legacy | 3.0, 3.2, 3.5, 3.7, 97, 2000 | Four typed/length pools; platform byte order; 16-byte other/detail rows, 38-byte entries, raw text | Container and physical rows solved; tag fields incomplete |
| DCL-compressed legacy | 2001–2006 | Platform-byte-order typed/length blocks; CRC-32; PKWARE DCL; fixed 16-byte other/detail rows and 38-byte entry rows | Container, codec, and physical pool rows solved; logical field mapping incomplete |
| Typed zlib transition | 2007–2008 | Four typed blocks; 2007 is mixed big/little endian, 2008 mostly little endian | Wrapper solved; records partly solved |
| Typed zlib stable | 2009, 2010, 2012 | Same four principal blocks and CRC validation; little endian in all but transition exceptions | Best implementation target |

The release boundary is not absolute. Of 108 Finale 2007 files, 81 validate as big-endian wrappers and 27 as little-endian; all report `MAC` except one little-endian Windows file. Of 182 Finale 2008 files, 180 are little-endian and two big-endian. Therefore version plus an observed wrapper/CRC test is safer than version alone.

## Platform coverage risk

The corpus is overwhelmingly Macintosh-derived. Header platform tuples, classic Mac resource forks, Macintosh archive containers, and Mac-originated conversion history dominate the evidence. Four banner-era Windows files nevertheless establish one concrete platform difference: three Finale 3.0 files and one Finale 2000 file serialize the otherwise identical four-pool container and fixed rows little-endian, while 185 recognized Mac-era samples are big-endian. This does not resolve Windows string, option, padding, or later-era behavior; Windows remains a separate validation axis before declaring a parser cross-platform.

## Checksums, compression, and wrapping

- 2007+: zlib plus explicit CRC-32 and stored block length, confirmed.
- 2001–2006: PKWARE DCL streams, decoded with `blast`; CRC-32 and stored block length in the file's detected byte order, confirmed.
- 3.x–2000: uncompressed typed pools with no identified checksum; fixed rows and text framing confirmed.
- Coda-banner/Finale 2: separate organization.

No evidence of whole-file encryption was found. No checksum was identified in the pre-2001 eras.
