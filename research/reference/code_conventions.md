# C++ code conventions

**Covers:** Naming, file headers, namespaces, preprocessor comments, Windows macro safety, MSVC directory-wide flags, and unity-build cleanliness.
**Read when:** Writing or reviewing any project-owned C++ source file.
**Confidence:** project rule.

- Follow the surrounding musxdom C++ conventions where this repository has not
  yet established a local style.
- Match musxdom's code naming: use `camelCase` for methods, properties, and
  variables, and `PascalCase` for classes and enums. Match denigma's filename
  convention by using `snake_case` rather than kebab-case for new source files.
- Begin every project-owned C++ header and source file with
  `Copyright (c) 2026 Robert G. Patterson` and the SPDX identifier `MIT`.
  Preserve original copyright and license notices in third-party sources.
- Use explicit nested namespace blocks rather than concatenated namespace
  declarations.
- End every preprocessor conditional with a comment naming the condition, such
  as `#endif // defined(FINALE_MUS_READER_ENABLE_INSTRUMENTATION)`. For
  `#ifdef` and `#ifndef`, spell the comment as the corresponding positive or
  negated condition.
- Do not require `NOMINMAX`. Protect standard-library `min` and `max` tokens
  from the Windows macros with parentheses, such as `(std::min)(a, b)` and
  `(std::numeric_limits<T>::max)()`.
- Compile every C and C++ object with `/bigobj` under MSVC. Keep this as a
  directory-wide build invariant so template-heavy musxdom factory
  instantiations cannot exceed the default COFF section limit in any target.
- Compile every object with `/utf-8` under MSVC, also directory-wide. Sources
  carry UTF-8 string literals and no byte-order mark; without the flag MSVC reads
  them in the machine's active code page and silently produces different bytes.
- Keep every project-owned translation unit unity-build clean. Unity compilation
  is the normal build for the library, tests, and recovery-coverage probe. CMake
  may combine unrelated source files into one
  translation unit, so an anonymous namespace does not make file-local names
  unique after amalgamation; use distinctive names for aliases, helpers, and
  constants when needed. Do not let unity-only fixes change runtime behavior.
- Project-owned targets enable unity compilation themselves; external dependencies
  retain their own build policy.

## Import reporting

Class importers use the private lazy boundary in
[`reporting.h`](../../src/import/support/reporting.h). Put substantial reporting passes in
named `report…` helpers with ordinary decoder or DOM types in their signatures. Small isolated
reports may use `withReporting` inline. Instrumentation build directives and token-erasing report
macros belong outside class importers; `scripts/check_reporting_boundary.py` checks this boundary.

A reporting callback is generic: use its writer for instance keys, origins, field metadata, and
report access. Those names must depend on the callback parameter, because a non-instrumented
build does not declare the public instrumentation types. Use `defaultField` and `behaviorField`
for singleton options, `unmappedField` to preserve existing provenance, and `report()` for the
remaining public report operations. All report-only formatting, allocation, lookups, and loops
belong inside the callback, including preparation of arguments to subordinate reporting helpers.
Capture existing objects by reference; capture initializers execute even when the callback does
not. Callbacks run synchronously and must not perform document mutations, required validation,
or user diagnostics.

`ReportClass` and `ReportInstance` retain identities across callbacks; `DeferredFieldReport`
retains a field awaiting reference resolution. Class-specific accumulated metadata uses
`ReportState<State>`, where `State` is a template with writer-dependent instrumentation types.
These wrappers have no payload with instrumentation disabled. Do not create parallel origin enums
or recompute decoding decisions to construct reports; pass the decoder's existing outcomes.

Validate reporting changes with the instrumented suite and a complete non-instrumented library
build. `FINALE_MUS_READER_BUILD_REPORTING_TESTING=ON` enables the standalone `reporting` test in
either configuration without requiring the coverage suite or an XML backend. The callback body is
never executed in disabled builds; removing trivial helper calls remains an optimization concern.
