# Where the no-duplication rule applies

**Covers:** The recovery-coverage probe/report pair as one pipeline, and which directories the single-implementation rule binds.
**Read when:** Working in `tools/coverage/`, `scripts/`, or tests, and deciding whether a repetition is a defect.
**Confidence:** project rule.

The rule also applies across the recovery-coverage pair:
`tools/coverage/recovery_coverage_probe.cpp` and its dependencies capture the observations, and
`scripts/recovery_coverage_report.py` classifies and aggregates them. They are
one analysis pipeline, not independent probes. A field name, path convention,
classification rule, transformation, or aggregate must have one authoritative
implementation; if the report needs information from the probe, add it to the
probe's structured output rather than re-deriving it from incidental fields.

This applies to `src/`, `include/`, `tests/evidence/` fixtures, and
`tools/coverage/`. It does not apply to the rest of `tools/`, `scripts/`, or
test code, which may repeat themselves as freely as makes sense — a probe is
meant to be written quickly while a question is live, and a test that spells
out its own expectations is clearer than one that shares a helper with the
code under test. `tools/coverage/` is the exception within `tools/`: it is
the one probe meant to stay comprehensible and regression-safe over time
rather than rewritten on demand, so its comments follow production discipline
even though its code may still repeat itself as freely as any other probe.
