# StaffOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Staff options

**Implemented in full.** From the arrival of group-name positioning, the default full and
abbreviated positions for staff and group names are four numeric globals at comparator `65534`.
Each supplies horizontal and vertical EVPUs and a packed flag word:

| Destination | Selector | Horizontal | Vertical | Flags |
|---|---:|---:|---:|---:|
| `namePos` | `04` | word 0 | word 1 | word 5 |
| `namePosAbbrv` | `66` | word 0 | word 1 | word 5 |
| `groupNameFullPos` | `79` | word 0 | word 1 | word 2 |
| `groupNameAbbrvPos` | `80` | word 0 | word 1 | word 2 |

The staff flag uses bits 0--1 for justification and bits 4--5 for horizontal alignment; the
group flag uses bits 0--2 and 3--4 respectively. Bit 15 is Expand Single Word in both. These
values map directly to musxdom's left/right/center enumeration. In the zlib epoch, the same
selectors use the repository-wide numeric-global class transformation.

Finale 3.0--3.5 uses an earlier six-word layout for selectors `04` and `66`:

| Word | Meaning |
|---:|---|
| 0 | horizontal offset |
| 1 | stored vertical offset |
| 2 | font id |
| 3 | point size |
| 4 | font effects |
| 5 | justification |

Within the uncompressed epoch, the simultaneous absence of group-position selectors `79` and `80`
selects this layout. The marker directly expresses the record-family change and also handles the
unrepresented Finale 3.3--3.4 interval; a damaged later file missing both selectors would be
misidentified as early. The Finale 3.7 addendum supplied for this investigation identifies group
names, including their positioning, as a 3.7 enhancement, agreeing with the observed structural
boundary.

The stored vertical offset is not in the later baseline coordinate system. Its conversion depends
on font metrics that the reader cannot reproduce reliably, so the importer applies the uniform
approximation `stored vertical + 3 * point size` and reports `LegacyMusAdjusted`. This exactly
matches 34 Times-font companion values across 17 Finale 3.0--3.5 documents. Six Times New Roman
positions across three Windows documents remain seven Efix below their companions; those residual
differences are categorized as `font-metric-approximation`. The stored justification is recovered
directly. Horizontal alignment follows that justification as source-era behavior when it differs
from the pinned value, while matching alignment and Expand Single Word remain `Finale27Default`.
The absent group positions likewise remain pinned defaults. **Strong for the layout and simple
approximation; open for Finale's actual font-metric conversion.**

The field semantics and flag packing are **public-PDK-derived** from
[`FCStaffNamePositionPrefs`](https://pdk.finalelua.com/class_f_c_staff_name_position_prefs.html),
[`FCGroupNamePositionPrefs`](https://pdk.finalelua.com/class_f_c_group_name_position_prefs.html),
and their [published header source](https://pdk.finalelua.com/ff__prefs_8h_source.html), accessed
2026-09-01. The selectors and word locations are **private-framework-derived**. Existing ETFs
independently contain all four selectors from Finale 3.7.2 onward. The controlled Finale 1.0.0
and 2.6.3 ETFs contain selector `04`, but the Coda UI exposes no name-position preferences and
does not yet have group names. The early selector's words therefore are not interpreted as the
later `namePos` object. Those early files do not contain selectors `66`, `79`, or `80`.

In the Coda epoch, full and abbreviated staff names use fixed horizontal offset `-192`, vertical
offset `-27`, left justification, and left alignment. These values differ from the pinned baseline
and report `LegacyBehavior`. Expand Single Word and every group-name position leaf already have the
effective Coda values in the pinned baseline and remain `Finale27Default`. The vertical value is
the predominant recoverable Coda behavior: tracked companions retain `-27` in 43 occurrences but
upgrade it to `-24` once and `-22` in 24 occurrences. Those two transformations are categorized as
`different_defaults`, not as separately recoverable source behaviors. The broader corpus likewise
contains seven Coda documents whose companions replace the fixed horizontal value `-192` with one
of `-152`, `-160`, `-200`, `-216`, `-228`, `-232`, or `-320`, identically for full and abbreviated
names. These horizontal transformations are categorized as `different_defaults` under the same
rule. **Strong.**

The three scalar members are the tail of numeric-global selector `97`, class `0x006f` in the zlib
epoch:

| Destination | Word |
|---|---:|
| `staffSeparation` | 12 |
| `staffSeparIncr` | 13 |
| `autoAdjustStaffSepar` | 14 |

The record's payload states whether the tail exists. Finale 2008 records are 36 bytes and carry all
three words; the tracked Finale 2007 record is only 24 bytes and ends before them. The controlled
Finale 2012 edit changes words 12--14 from `-320`, `72`, `1` to `-289`, `71`, `0`, exactly matching
the independently parsed companion. Recovery therefore requires at least 15 payload words rather
than a recovered product version. This is **strong** from the controlled Finale 2012 edit, the
short Finale 2007 record, and the consistent Finale 2008 record shape. Files without the tail have
no exposed source-era setting. On that side of the same structural gate, `staffSeparation` receives
the fixed value `-320` as `LegacyBehavior`; `staffSeparIncr` and `autoAdjustStaffSepar` retain their
`Finale27Default` values.

`indivPos` and `hidden` are not persisted in this options context. They remain false from the
pinned baseline and report `Finale27Default`. Outside the Coda epoch, a missing located
name-position selector likewise reports `Finale27Default` for the five fields that selector would
have supplied.
