#!/bin/bash

# runs 'meson test' for all build subdirectories
set -e
cd `git rev-parse --show-toplevel`

for D in build/*/ ; do
    echo
    echo "-- building $D --"
    meson test -q --print-errorlogs -C $D
done
