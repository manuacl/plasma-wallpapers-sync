#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
# SPDX-License-Identifier: CC0-1.0
#
# Build the project inside the KDE 6.10 Flatpak SDK and run the test
# suite. Designed for rpm-ostree atomic hosts (Bazzite, Silverblue) so
# nothing is installed on the base layer.
#
# Pass extra flatpak-builder flags after `--`, e.g.:
#   dev/build.sh --                # full build + tests + install in .flatpak-app/
#   dev/build.sh -- --build-only   # build + tests, skip install step
#   dev/build.sh -- --run ctest    # re-run tests against existing build tree
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

BUILD_DIR="${PWS_BUILD_DIR:-.flatpak-app}"
STATE_DIR="${PWS_STATE_DIR:-.flatpak-builder}"
MANIFEST="dev.manuacl.plasmawallpapersync.yaml"

exec flatpak run --filesystem="$REPO_ROOT" org.flatpak.Builder \
    --user \
    --force-clean \
    --ccache \
    --keep-build-dirs \
    --state-dir="$STATE_DIR" \
    "$BUILD_DIR" \
    "$MANIFEST" \
    "$@"
