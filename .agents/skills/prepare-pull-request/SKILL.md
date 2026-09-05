---
name: prepare-pull-request
description: Validate and deliver a feature branch through a pull request in this repository. Use whenever the user asks to open or create a PR, or asks to commit and push changes that will immediately become a PR. Requires the local non-instrumented library-only build; routine instrumented variants may be left to CI.
---

# Prepare a pull request

Use this workflow only after the user has authorized the requested commit, push, and pull-request
operations. That authorization does not cover unrelated changes or cleanup.

## Final local validation

After the implementation, tests, evidence, and documentation have stabilized, and immediately
before delivery:

1. Read `.github/workflows/build_and_test.yml` and locate its `non-instrumented-build` job. That job
   is the source of truth for the required CMake options and build target.
2. Configure a separate build directory with the same library-only, tools-off, tests-off, and
   instrumentation-off settings. Adapt only generator and compiler selection when the local host
   cannot reproduce the Linux runner; on macOS, use the available local toolchain.
3. Build the same `finale_mus_reader` target. An existing instrumented build directory does not
   satisfy this check.
4. If the build fails, stop delivery, fix it, and rerun this validation after the changes stabilize.

Do not rerun an instrumented build or test suite solely as final PR ceremony. Those variants are
exercised throughout development and may receive their final run in CI. Report which relevant
instrumented tests were run during development, but the non-instrumented library build is the one
mandatory local build at this boundary.

## Deliver the branch

1. Inspect `git status --short`, the complete intended diff, and `git diff --check`. Preserve
   unrelated worktree changes.
2. Stage only the intended files and review the staged status and diff check.
3. Commit and push only when authorized by the user. Never deliver directly from `main`.
4. Open the pull request against the intended base branch and verify its URL, title, head, base,
   and open state.
5. Report the branch, commit, validation result, and pull-request link.
