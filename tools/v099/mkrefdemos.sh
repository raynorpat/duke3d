#!/bin/sh
# Author routes, verify them against DOS, and install only matching references.
set -eu
exec python3 "$(dirname "$0")/mkrefdemos.py" "$@"
