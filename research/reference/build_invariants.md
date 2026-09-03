# Build invariants

**Covers:** Why unity compilation is a project-owned-target invariant, how a dependency opts out, and the MSVC flags that are directory-wide rather than per-target.
**Read when:** Changing `CMakeLists.txt`, adding a dependency, or diagnosing a unity-build-only failure.
**Confidence:** project rule.

**A unity build is this project's invariant and must never
be imposed on code this repository does not own.** `CMakeLists.txt` states that rule once, as
`finale_mus_reader_keep_out_of_unity()`, and calls it at each dependency: the zlib targets,
musxdom, pugixml, Catch2's companion target, and the pinned `blast` source. zlib is the
standing example of why -- its `inftrees.h` is included by several of its own sources and is
not idempotent -- but the rule is about ownership, not about zlib, and a dependency added
later is opted out by calling that function rather than by remembering a policy written
elsewhere. Catch2 itself is the one deliberate exception: `tests/CMakeLists.txt` amalgamates
it for build speed, which is a choice about one dependency rather than a consequence of the
project-owned target policy.

In a normal build, no dependency target should have a `Unity/` source: only
`finale_mus_reader`, `finale_mus_reader_tests`, `recovery_coverage_probe` when tools are
enabled, and the deliberate Catch2 should appear.
