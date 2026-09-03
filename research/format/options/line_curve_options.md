# LineCurveOptions

**Covers:** Legacy record locations, epoch and version gates, and companion-verification results for this class.
**Read when:** Implementing, extending, or debugging recovery of this class.
**Confidence:** mixed -- each finding carries its own label; see inline.

## Line and curve options

The later fixed-row layout recovers eleven of `LineCurveOptions`' thirteen fields, and the zlib
layout retains the same payload organization under the numeric-global class transform. The
locations are:

| musxdom member | fixed selector and word | stored form |
| --- | --- | --- |
| `bezierStep` | `15` word 4 | signed word |
| `enclosureWidth` | `27` word 3 | Efix word |
| `staffLineWidth` | `58` word 5 | Efix word |
| `legerLineWidth` | `59` word 0 | Efix word |
| `legerFrontLength`, `legerBackLength` | `59` words 1, 2 | EVPU words |
| `restLegerFrontLength`, `restLegerBackLength` | `01` words 2, 3 | EVPU words |
| `psUlDepth`, `psUlWidth` | `62` long words 0, 2 | high-word-first, divided by 10,000 |
| `pathSlurTipWidth` | `97` long word 0 | high-word-first, divided by 10,000 |

These locations are **private-framework-derived** and independently agree with the record shapes
in the controlled evidence. The public `FCSizePrefs` API supplies the semantic types for enclosure,
staff, ledger, and Shape Designer slur-tip widths; `FCMiscDocPrefs` supplies curve resolution and
PostScript underline depth and thickness. See the public
[`FCSizePrefs`](https://pdk.finalelua.com/class_f_c_size_prefs.html) and
[`FCMiscDocPrefs`](https://pdk.finalelua.com/class_f_c_misc_doc_prefs.html) documentation, accessed
2026-08-29. The source locations were checked against the authorized read-only
Framework preference tables on 2026-08-30. No declaration or source text
was copied. Confidence is **strong** for the table as a whole and **confirmed** where the controlled
Finale 2.6.3 curve-options pair varies the value.

Availability is structural rather than date-only. Selector `62` is already present in the
Finale 2.6.3 evidence but absent from Finale 1.0.0. Where it is present, its two PostScript underline
values use the later ten-thousandths representation. Without selector `62`, Coda stores the same
fields as the first two single-precision values of selector `52`. The controlled Finale 1.0.0 edit
moves them from -0.25 and 0.0416 to -0.37 and 0.0713 in both MUS and ETF, while Finale 27 discards
the edits and retains -1.5 and 0.5. Selector presence therefore selects the representation without
depending on a recovered version. Selector `97` and meaningful selector-`01` rest-ledger lengths
appear only in the compressed evidence. All 38 tracked uncompressed companions use rest-ledger
length 3 while their selector-`01` words are zero, so the importer leaves the pinned baseline's
matching value rather than claiming those zero words. DCL and zlib files recover the later stored
values directly.

Opening that edited Finale 1 document and saving it in Finale 2.6.3 retains selector `52`'s
original float bits and adds selector `62` with fixed-point values -3700 and 712. The corresponding
semantic values are -0.37 and 0.0712, and Finale 27 preserves both. The one-ten-thousandth width
loss comes from truncating the single-precision value just below 0.0713, not from the factor-three
presentation used by the Finale 2.6.3 UI.

Direct Finale 1.0.0 upgrades discard both recovered selector-52 values. Recovery coverage therefore
classifies the two source-owned disagreements in each of the 38 tracked Finale 1.0.0 fixtures as
`finale-upgrade-loss`. The rule is limited to that release, `LegacyMus` origin, and the two underline
paths; the Finale 2.6.3 resave's selector-62 values compare equal and are not classified. A future
non-Finale-1 source without selector `62` remains visible until its upgrade behavior is established.

Coda curve resolution uses its own zero sentinel. All 38 tracked Finale 1.0.0 documents store zero
at selector `15` word 4 and upgrade to 16; the controlled Finale 2.6.3 edit changes an explicit 16
to 33 in both MUS and ETF and the modern companion preserves 33. The importer therefore reports a
zero Coda word as sixteen-step `LegacyBehavior` and preserves any nonzero word as `LegacyMus`.

Rounded-enclosure and corner-radius controls are MUSX-only features: neither the public PDK surface
nor the authorized legacy preference tables contains either field. The corresponding legacy
behavior is square corners and radius zero, which every one of the 173 tracked companions across
all four epochs preserves. The importer therefore sets `enclosureRoundCorners = false` and
`enclosureCornerRadius = 0` as **confirmed** `LegacyBehavior`; these are not claimed as stored
fields.

Coda predates the enclosure-, staff-, and ledger-line width controls available by Finale 3.7.2.
The source locations used later are absent or have colliding meanings there. Upgrading the
controlled Finale 1.0.0 line-options document through Finale 3.7.2 assigns 118 to all three
fields. Independently, the Finale 3.7.2 baseline stores 118 at selector `27` word 3, selector `58`
word 5, and selector `59` word 0. The importer therefore assigns the common value 118 as
`LegacyBehavior`; it differs from the pinned Finale 27 value 115, while a historical behavior
matching that baseline would remain `Finale27Default`. All 60 tracked direct-to-Finale-27 Coda
companions instead contain 224 for all three fields, so coverage classifies only the exact Coda
`LegacyBehavior` transition 118 to 224 as `different_defaults`. **Confirmed.**

The first authorized full-corpus capture for this class exposed the corresponding early
uncompressed boundary. Seventeen distinct Finale 3.0-3.2 sources have no selector `27`; the reader
therefore retained the pinned value 115, while every companion explicitly stores
`enclosureWidth = 224`. A representative source record stream confirms selector `27` is absent,
and the representative raw EnigmaXML confirms 224 is stored rather than synthesized by musxdom.
Selector `27` presence is consequently the structural boundary in the uncompressed epoch: when it
is absent, `enclosureWidth` receives the same historical value 118 as `LegacyBehavior`; when it is
present, word 3 remains authoritative. Coverage classifies only the exact legacy-behavior value
118 to companion 224 as `different_defaults`. The other two widths remain source-controlled in
that epoch. **Strong:** the seventeen sources agree, but no controlled Finale 3.0-3.2 UI fixture
varies or directly displays the historical enclosure width.

The other five Coda geometry fields whose later selectors are absent remain at the pinned defaults
and agree in the tracked capture.
