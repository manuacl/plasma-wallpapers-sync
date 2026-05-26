---
name: finish-branch
description: Audit a plasma-wallpapers-sync branch (REUSE, file-size cap, ternaries, layering invariants, Flatpak SDK build + ctest) THEN push and open the PR if everything is green. Trigger when the user says "finis la branche", "prêt à merger ?", "/finish-branch", "ouvre la PR".
---

# Finish branch — plasma-wallpapers-sync

Full "branch → opened PR" pipeline:

1. Local audit (CLAUDE.md rules + REUSE + build + tests reproduced).
2. If **everything is green**: push + `gh pr create`.
3. If **any check is red**: stop, do not push, report what needs fixing.

The goal is **zero surprise** when CI runs and **zero PR opened on red**.

## When to use

- The user says "finis la branche", "prêt à merger ?", "audit branche",
  invokes `/finish-branch`, or asks to open a PR.
- Before a `git push` on an open PR (or before opening one).

## When NOT to use

- For a WIP commit that will be squashed later — this skill audits the
  final state, not each intermediate step.
- The branch only touches `docs/` or `CLAUDE.md` and the build doesn't
  need re-running — skip phase A.5 (build + tests) and go straight to
  the lightweight checks + push.

## Procedure

**Phase A — audit**: run checks 1 → 6 in order. Stop at the first
block that surfaces red. Report PASS / FAIL with useful details.

**Phase B — push + PR + version tag**: only fire if phase A is fully
green.

### 1. REUSE compliance

Reproduces what CI's `reuse` job checks: every committed file is
covered either by a header SPDX line or by an aggregate/override
entry in `REUSE.toml`.

```bash
if command -v reuse >/dev/null; then
    reuse lint || status=1
else
    echo "(reuse not installed locally; CI will run it — skipping)"
fi
```

### 2. File-size cap — 500 lines

CLAUDE.md § Working rules sets the cap. The check runs against
committed source under `src/` and `tests/`. Docs and CLAUDE.md are
intentionally not capped.

```bash
MAX=500
status=0
shopt -s nullglob globstar
for f in src/**/*.cpp src/**/*.h src/**/*.qml tests/**/*.cpp tests/**/*.h; do
    lines=$(wc -l < "$f")
    if [ "$lines" -gt "$MAX" ]; then
        echo "FAIL: $f = $lines lines (> $MAX)"
        status=1
    fi
done
[ $status -eq 0 ] && echo "PASS: file-size (≤500 lines)"
```

### 3. No nested ternaries

Heuristic: a line with two `?` and two `:` is suspect. False positives
possible (e.g. `foo ? "a:b" : "c"`); re-read each match.

```bash
grep -nE '\?[^?]*\?[^:]*:[^:]*:' \
    src/**/*.cpp src/**/*.h src/**/*.qml \
    tests/**/*.cpp tests/**/*.h \
    2>/dev/null && echo "WARN: possibly nested ternaries (re-read matches)"
```

### 4. Layering invariants

Two of the four CLAUDE.md layering rules need a grep — the other two
(C++ link-time bans on `core/`) are enforced by the
`_pws_enforce_core_isolation` CMake guard and re-trip at configure
time in check 5.

**4a. `src/qml/core/` cannot `import org.kde.*`** (the plasma-isolation
seam for QML — same shape as ring-monitor's invariant on
`contents/ui/core/`).

```bash
if [ -d src/qml/core ]; then
    forbidden=$(grep -rnE 'import org\.kde\.' src/qml/core/ 2>/dev/null || true)
    if [ -n "$forbidden" ]; then
        echo "$forbidden"
        echo "FAIL: src/qml/core/ imports org.kde.* (plasma-isolation invariant)"
        echo "  fix: wrap the Kirigami/Plasma-bound type in src/qml/platforms/plasma/<Name>.qml"
        status=1
    else
        echo "PASS: src/qml/core/ free of org.kde.* imports"
    fi
fi
```

**4b. `src/core/` cannot `#include <QDBus...>`, `#include <KAuth/...>`
or `#include <Kirigami/...>`** (the C++ side of the same seam). The
CMake link-time guard catches this if the corresponding library is
listed in `target_link_libraries`, but a stray `#include` that doesn't
yet trigger linking is still a violation — flag it early.

```bash
forbidden=$(grep -rnE '#include[[:space:]]+<(QDBus|KAuth|Kirigami)' src/core/ 2>/dev/null || true)
if [ -n "$forbidden" ]; then
    echo "$forbidden"
    echo "FAIL: src/core/ includes a forbidden header (plasma-isolation invariant)"
    status=1
else
    echo "PASS: src/core/ free of QDBus/KAuth/Kirigami includes"
fi
```

### 5. Build + tests via the Flatpak SDK

This is the slow step (~30s incremental with ccache, ~2min cold).
Reproduces both CI jobs (configure + build + ctest) in the same
environment plasma-shell ships against (Qt 6.10 / KF6 6.x).

```bash
./dev/build.sh -- --build-only
```

A green run prints `100% tests passed, 0 tests failed out of N`. The
"Finishing app" / "Exporting share/applications/" lines at the end
are not failures — they're flatpak-builder's final bundle step.

### 6. Git state — blocking

```bash
# 6a. Clean working tree.
[ -z "$(git status --short)" ] || { echo "FAIL: uncommitted changes"; exit 1; }

# 6b. Not on main.
branch=$(git rev-parse --abbrev-ref HEAD)
[ "$branch" = "main" ] && { echo "FAIL: HEAD is on main, no PR possible"; exit 1; }

# 6c. Branch up to date with origin/main.
git fetch origin main --quiet
behind=$(git rev-list --count HEAD..origin/main)
[ "$behind" -gt 0 ] && { echo "FAIL: branch behind origin/main by $behind commit(s) — rebase first"; exit 1; }
```

### 7. Push + open the PR (phase B)

**Only run if 1 → 6 are all green.**

```bash
# 7a. Gather context.
git log --oneline origin/main..HEAD
git diff origin/main...HEAD --stat
gh pr view --json url 2>/dev/null && echo "PR already open — push only, no re-create"

# 7b. Push.
git push -u origin HEAD

# 7c. If a PR exists, the push updated it. Capture the number either way.
if pr_url=$(gh pr view --json url --jq .url 2>/dev/null); then
    pr_number=$(gh pr view --json number --jq .number)
else
    gh pr create --title "<concise title>" --body "$(cat <<'EOF'
## Summary
<1-3 bullets: what the PR changes and why>

## Test plan
- [x] REUSE compliance (`reuse lint`)
- [x] file-size cap (≤500 lines)
- [x] no nested ternaries
- [x] plasma-isolation invariant (src/qml/core/ no org.kde.*, src/core/ no QDBus/KAuth/Kirigami)
- [x] Flatpak SDK build (`./dev/build.sh -- --build-only`)
- [x] `ctest` (<N/N>)
- [ ] <visual check if UI: launch the binary in the SDK>

(Decompose each phase-A check into its own bullet. Skip CI green —
it's mandatory on this repo, listing it adds noise.)
EOF
)"
    pr_url=$(gh pr view --json url --jq .url)
    pr_number=$(gh pr view --json number --jq .number)
fi
echo "PR: $pr_url (#$pr_number)"
```

### 8. Version bump label — delegate to `bump-label` skill

Hand off to `.claude/skills/bump-label/` with:

- `pr_number`: from step 7
- `current_version`: `grep -oP '^[[:space:]]+VERSION[[:space:]]+\K[0-9]+\.[0-9]+\.[0-9]+' CMakeLists.txt | head -1`
- `changed_files`: `git diff --name-only origin/main...HEAD`
- `commits`: `git log --format='%B' origin/main..HEAD`

The label is consumed by `.github/workflows/version.yml`: on merge to
main, it reads the PR's `bump:*` label, bumps `CMakeLists.txt`'s
project `VERSION` accordingly, commits the change and tags `vX.Y.Z`.
With no `bump:*` label, the workflow exits cleanly.

Return the PR URL **and** the chosen bump level (or "no bump") to the
user.

## Expected report

Summary table at the end of phase A:

```
✓ REUSE compliance
✓ file-size (≤500 lines)
✓ no nested ternaries
✓ src/qml/core/ free of org.kde.* imports
✓ src/core/ free of QDBus/KAuth/Kirigami includes
✓ Flatpak SDK build (8/8 test suites passed)
✓ git: clean working tree, up to date with origin/main
```

- **All green** → chain into phase B (push + PR + bump label), then
  return the PR URL and the chosen bump: "PR opened: <url> (tagged
  bump:minor)".
- **Any FAIL** → **do not push, do not create a PR**. Cite the
  file:line, suggest the precise fix, do not apply the fix
  automatically — let the user validate.

## Why this procedure

- **Reproducing CI locally** avoids the push → CI red → fix → re-push
  round-trip. The build is slower than ring-monitor's QML-only checks
  (~30s vs ~3s), but still cheaper than a CI cycle.
- **Auditing non-mechanical rules** (no nested ternaries, plasma-
  isolation grep) catches drift before CI even sees the commit.
- **Stopping at the first red block** avoids drowning the user; the
  next block may depend on the previous one (e.g. tests failing
  because the build didn't link).
