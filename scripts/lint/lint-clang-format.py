#!/usr/bin/env python3

from glob import glob
from pathlib import Path
from subprocess import run
from os import path
import subprocess
import sys
from time import time
import re

root_path = path.abspath(Path(__file__).parent.parent.parent)

extensions = ["h", "cpp"]
exclusions = ["nanobench\.h"]

files = []
for ext in extensions:
    g = glob(f"{root_path}/src/**/*.{ext}", recursive=True)
    files.extend(g)

# filter out exclusions
for exclusion in exclusions:
    l = filter(lambda file: re.search(exclusion, file) == None, files)
    files = list(l)


command = ['clang-format', '--dry-run', '-Werror'] + files
p = subprocess.Popen(command,
                     stdout=subprocess.PIPE,
                     stderr=None,
                     stdin=subprocess.PIPE,
                     universal_newlines=True)

stdout, stderr = p.communicate()
if p.returncode != 0:
    sys.exit(p.returncode)

print(f"clang-format checked {len(files)} files")
