# LayerAttributes

**Covers:** Layer rest offsets and how often the field is actually recovered rather than supplied by the pinned baseline.
**Read when:** Working on layer attributes or interpreting their coverage numbers.
**Confidence:** partial; the class is a stub excluded from the completeness audit.

**Layer rest offsets.** `LayerAttributes.restOffset` differs from the companion on 82 counts
across the tracked corpus, every one of them where the reader recovered nothing and the pinned
baseline supplied the value. That is a statement about the baseline rather than about a decoder,
but it means the field is unrecovered far more often than the class's coverage suggests.
