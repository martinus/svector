#!/usr/bin/env python3

"""Check that every file repeating the version agrees with the ANKERL_SVECTOR_VERSION_* macros
in svector.h, which are the reference.

With --expect it also checks that reference against a version passed in. That is how the release
workflow ties a pushed tag to what the tagged commit actually contains: the three files agreeing
with each other says nothing about whether v1.2.1 was put on the commit that says 1.2.1."""

import argparse
import os
import pathlib
import re


parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
parser.add_argument("--expect", default=None, metavar="X.Y.Z", help="also require this exact version, e.g. 1.2.1")
args = parser.parse_args()

root = os.path.abspath(pathlib.Path(__file__).parent.parent.parent)

# filename, pattern, number of occurrences
file_pattern_count = [
    (
        f"{root}/meson.build",
        r"version: '(\d+)\.(\d+)\.(\d+)'",
        1),
    (
        f"{root}/include/ankerl/svector.h",
        r"Version (\d+)\.(\d+)\.(\d+)\n",
        1),
    (
        f"{root}/CMakeLists.txt",
        r"^\s+VERSION (\d+)\.(\d+)\.(\d+)\n",
        1),
]

# let's parse the reference from svector.h
major = "??"
minor = "??"
patch = "??"
with open(f"{root}/include/ankerl/svector.h", "r") as f:
    for line in f:
        r = re.search(r"#define ANKERL_SVECTOR_VERSION_([A-Z]+) (\d+)", line)
        if not r:
            continue

        if "MAJOR" == r.group(1):
            major = r.group(2)
        elif "MINOR" == r.group(1):
            minor = r.group(2)
        elif "PATCH" == r.group(1):
            patch = r.group(2)
        else:
            "match but with something else!"
            exit(1)

is_ok = True
for (filename, pattern, count) in file_pattern_count:
    num_found = 0
    with open(filename, "r") as f:
        for line in f:
            r = re.search(pattern, line)
            if r:
                num_found += 1
                if major != r.group(1) or minor != r.group(2) or patch != r.group(3):
                    is_ok = False
                    print(f"ERROR in {filename}: got '{line.strip()}' but version should be '{major}.{minor}.{patch}'")
    if num_found != count:
        is_ok = False
        print(f"ERROR in {filename}: expected {count} occurrences but found it {num_found} times")

if args.expect is not None and args.expect != f"{major}.{minor}.{patch}":
    is_ok = False
    print(f"ERROR: expected version '{args.expect}' but svector.h says '{major}.{minor}.{patch}'")

if not is_ok:
    exit(1)
