#!/usr/bin/env python3
"""Enforce the startup documentation word budget.

Agents read exactly four files at the start of a session. Their combined size is the
project's automatic context cost, so it is a build-checkable invariant rather than a
convention. See `.agents/skills/maintain-documentation/SKILL.md`.
"""
import os
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

STARTUP_SET = (
    "AGENTS.md",
    "research/ORIENTATION.md",
    "research/STATE.md",
    "research/INDEX.md",
)
TOTAL_LIMIT = 4000


def main() -> int:
    total = 0
    rows = []
    missing = []
    for rel in STARTUP_SET:
        path = os.path.join(REPO, rel)
        if not os.path.exists(path):
            missing.append(rel)
            continue
        words = len(open(path, encoding="utf-8").read().split())
        rows.append((rel, words))
        total += words

    for rel, words in rows:
        print("%-28s %5d words" % (rel, words))
    print("%-28s %5d words  (limit %d)" % ("TOTAL", total, TOTAL_LIMIT))

    if missing:
        print("\nMISSING startup file(s): " + ", ".join(missing), file=sys.stderr)
        return 1
    if total > TOTAL_LIMIT:
        print(
            "\nThe startup set is %d words over budget. Move material into the file that owns\n"
            "the subject under research/, not into another startup file, and do not raise the\n"
            "limit. See .agents/skills/maintain-documentation/SKILL.md." % (total - TOTAL_LIMIT),
            file=sys.stderr,
        )
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
