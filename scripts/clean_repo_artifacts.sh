#!/usr/bin/env bash
set -euo pipefail

# Remove editor swap files, backup files, and local build artifacts that should not be committed.
# Safe to run from repository root.

ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$ROOT"

rm -f src/.main.c.swp
rm -f src/*.bak include/*.bak config/*.bak systemd/*.bak
rm -f *.tmp *.patch *.rej
rm -f events.log build.log

find . -name '*.swp' -type f -delete
find . -name '*~' -type f -delete

printf 'Repository cleanup completed.\n'
printf 'Remaining suspicious artifacts:\n'
find src include config systemd -maxdepth 2 \( -name '*.bak' -o -name '*.swp' -o -name '*~' \) -print 2>/dev/null || true
