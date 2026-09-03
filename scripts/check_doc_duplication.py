#!/usr/bin/env python3
"""Find prose that says the same thing in two documentation files.

Advisory, not a gate. `AGENTS.md` requires every fact to have one home, and
`.agents/skills/maintain-documentation/SKILL.md` asks you to grep for a distinctive phrase
before writing an explanatory paragraph. This is that grep, done systematically.

    python3 scripts/check_doc_duplication.py                # whole tree
    python3 scripts/check_doc_duplication.py --since main   # only paragraphs this branch adds

Prefer `--since`. Reporting the whole tree also surfaces the reference/investigation pairs,
where a conclusion and the experiment behind it legitimately share wording. `--since` reports
only paragraphs a file did not already have, which is where a second copy is actually created --
including a passage copied verbatim out of some other file.

Exits non-zero only with --strict.
"""
import argparse
import collections
import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SKIP_DIRS = {"build", "build-release", "private", "third_party", ".git", ".cache"}
SKIP_PREFIXES = (
    "research/history/archive/",   # verbatim pre-split copies, duplicated on purpose
    "research/corpora/",           # rendered by scripts/render_record_catalog.py and friends,
                                   # whose source is the script, not the markdown
)
# Templated per-file synopsis headers are meant to be parallel across files.
TEMPLATED = re.compile(r"^\*\*(Covers|Read when|Confidence):\*\*", re.M)


def normalize(block):
    """A paragraph as a comparable word list, or None if it is not prose."""
    if block.lstrip().startswith(("|", "#")) or TEMPLATED.search(block):
        return None
    text = re.sub(r"\[([^\]]*)\]\([^)]*\)", r"\1", block)   # link -> its label
    text = re.sub(r"[`*_>#|-]", " ", text)
    text = re.sub(r"[^a-z0-9 ]", " ", text.lower())
    return text.split()


def paragraphs(text):
    text = re.sub(r"```.*?```", "", text, flags=re.S)
    for block in re.split(r"\n\s*\n", text):
        words = normalize(block)
        if words:
            yield words, " ".join(block.split())[:100]


def doc_files():
    for root, dirs, files in os.walk(REPO):
        dirs[:] = [d for d in dirs if d not in SKIP_DIRS]
        for name in sorted(files):
            if not name.endswith(".md"):
                continue
            rel = os.path.relpath(os.path.join(root, name), REPO)
            if not rel.startswith(SKIP_PREFIXES):
                yield rel


def previously_in_file(ref):
    """Map each file to the normalized paragraphs it already held at `ref`.

    A paragraph counts as new when this file did not have it, not when the repository did
    not: copying an existing passage into a second file is the duplication that matters,
    and a repo-wide test would filter out exactly that case.
    """
    try:
        listing = subprocess.run(["git", "-C", REPO, "ls-tree", "-r", "--name-only", ref],
                                 capture_output=True, text=True, check=True).stdout.split()
    except subprocess.CalledProcessError:
        sys.exit("cannot read git ref %r" % ref)
    before = {}
    for rel in listing:
        if not rel.endswith(".md") or rel.startswith(SKIP_PREFIXES):
            continue
        blob = subprocess.run(["git", "-C", REPO, "show", "%s:%s" % (ref, rel)],
                              capture_output=True, text=True)
        if blob.returncode:
            continue
        before[rel] = {" ".join(words) for words, _ in paragraphs(blob.stdout)}
    return before


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--since", metavar="REF",
                    help="report only paragraphs whose wording is new relative to this git ref")
    ap.add_argument("--min-run", type=int, default=20, metavar="N",
                    help="shared run of N words counts as a repetition (default 20)")
    ap.add_argument("--strict", action="store_true", help="exit non-zero when anything is reported")
    args = ap.parse_args()
    run = args.min_run

    before = previously_in_file(args.since) if args.since else None

    index = collections.defaultdict(set)
    store = {}
    fresh = set()
    for rel in doc_files():
        path = os.path.join(REPO, rel)
        for i, (words, preview) in enumerate(paragraphs(open(path, encoding="utf-8").read())):
            if len(words) < run:
                continue
            store[(rel, i)] = preview
            if before is None or " ".join(words) not in before.get(rel, ()):
                fresh.add((rel, i))
            for j in range(len(words) - run + 1):
                index[" ".join(words[j:j + run])].add((rel, i))

    pairs = collections.Counter()
    for gram, locations in index.items():
        if len(locations) < 2:
            continue
        locations = sorted(locations)
        for a in range(len(locations)):
            for b in range(a + 1, len(locations)):
                if locations[a][0] == locations[b][0]:
                    continue
                if before is not None and not (locations[a] in fresh or locations[b] in fresh):
                    continue
                pairs[(locations[a], locations[b])] += 1

    for (left, right), count in pairs.most_common():
        print("%d shared runs of %d words:" % (count, run))
        print("   %s\n     %s" % (left[0], store[left]))
        print("   %s\n     %s" % (right[0], store[right]))
    scope = "new since %s" % args.since if args.since else "whole tree"
    print("%d repeated passages (%s, runs of %d words)" % (len(pairs), scope, run))
    if pairs:
        print("Give each fact one home and link to it from the other file, or say on the line "
              "why one home was not possible.")
    return 1 if (pairs and args.strict) else 0


if __name__ == "__main__":
    sys.exit(main())
