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
