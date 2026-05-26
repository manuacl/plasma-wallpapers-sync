# Plasma Wallpaper Sync

GUI tool to view and edit, independently or together, the four
background images KDE Plasma 6 stores in separate configs — desktop,
lock screen, login screen (plasmalogin), and session splash. Plasma's
built-in UI splits these across several settings panels with no shared
view; this tool brings them into one place.

Target environment: KDE Plasma 6 on Wayland, with **plasmalogin** as
display manager (not SDDM). Tested on Bazzite but should work on any
Plasma 6 distro with plasmalogin.

## Publication target

**store.kde.org as a standalone native app** is the v1 target —
distributed as AppImage and (later) native packages (COPR / AUR / KDE
neon). This choice is locked because it avoids Flatpak's sandbox
friction on `/etc/plasmalogin.conf` and lets KAuth work natively.

**Two future evolutions are deliberately kept reachable by the
architecture:**

1. **KCM integration** into Plasma System Settings (next to
   *Verrouillage d'écran*) — the architecturally correct long-term
   home for this feature.
2. **Flatpak / Flathub distribution** — once a sandbox-safe strategy
   for the privileged plasmalogin write is decided (`flatpak-spawn
   --host pkexec` to a host helper, or degraded read-only mode for
   that surface).

The implication for design: the **wallpaper-sync core logic must be
cleanly separable from (a) the standalone-app shell and (b) the
privileged-write mechanism.** Both evolutions become refactors of the
outer layers, not a rewrite. This mirrors the plasma-isolation seam
that `ring-monitor` uses, and the rules below enforce it.

## Stack

| Layer | Choice |
|---|---|
| Language | C++20 |
| UI | Qt6 + QML + Kirigami |
| Config I/O | KConfig (KF6) — never shell out to `kreadconfig6` / `kwriteconfig6` |
| Privileged write | KAuth (KF6) — helper subclass + generated polkit policy |
| Plasma reload | QtDBus → `org.kde.plasmashell`, `org.kde.screensaver` |
| Build | CMake + Extra-CMake-Modules (ECM) |
| i18n | KLocalizedString + l10n.kde.org workflow |
| Tests | QTest (C++ core), qmltestrunner (QML views) |
| Lint | clang-tidy + clang-format with KDE Frameworks style, qmllint |
| License | GPL-2.0-or-later, REUSE-compliant (SPDX headers) |

## File layout (target)

```
plasma-wallpapers/
├── CMakeLists.txt
├── CLAUDE.md                              ← you are here
├── README.md
├── LICENSES/                              ← REUSE compliance
├── data/
│   ├── dev.manuacl.plasmawallpapersync.desktop
│   ├── dev.manuacl.plasmawallpapersync.policy.in   ← polkit policy (KAuth-generated)
│   └── icons/
├── src/
│   ├── core/                              ← portable logic — NO Qt-GUI, NO Kirigami, NO KAuth, NO QtDBus
│   │   ├── WallpaperSurface.{h,cpp}       ← abstract: one surface (read current, propose new, apply)
│   │   ├── DesktopSurface.{h,cpp}         ← plasma-org.kde.plasma.desktop-appletsrc via KConfig
│   │   ├── LockscreenSurface.{h,cpp}      ← kscreenlockerrc via KConfig
│   │   ├── LoginSurface.{h,cpp}           ← /etc/plasmalogin.conf — reads directly, writes via PrivilegedWriter
│   │   ├── SplashSurface.{h,cpp}          ← v1.1
│   │   ├── PrivilegedWriter.h             ← abstract: write to a privileged path
│   │   └── SyncEngine.{h,cpp}             ← orchestrates "apply this image to subset {desktop, lockscreen, …}"
│   ├── privileged/                        ← implementations of PrivilegedWriter
│   │   ├── kauth/
│   │   │   └── KAuthPrivilegedWriter.{h,cpp}
│   │   └── flatpak/                       ← future evolution
│   ├── reload/                            ← Plasma reload signals (QtDBus only lives here)
│   │   └── PlasmaReloader.{h,cpp}
│   ├── helper/                            ← KAuth helper binary (separate executable, runs as root)
│   │   └── PlasmaLoginHelper.{h,cpp}
│   ├── shell/                             ← outer shells; v1 ships standalone, v2 adds kcm/
│   │   └── standalone/
│   │       └── main.cpp
│   └── qml/
│       ├── core/                          ← portable QML views — NO `import org.kde.*`
│       │   ├── Main.qml
│       │   ├── SurfaceCard.qml            ← one of the 4 cards (current preview + "select" button)
│       │   ├── ApplyTargetSelector.qml    ← the "which surfaces" checkboxes
│       │   └── ImagePicker.qml
│       └── platforms/
│           └── plasma/                    ← single home of `import org.kde.kirigami`
│               ├── Theme.qml              ← re-exposes Kirigami theme tokens
│               └── ThemedIcon.qml
├── tests/
│   ├── unit/                              ← QTest, no display server needed
│   │   ├── tst_DesktopSurface.cpp
│   │   ├── tst_LockscreenSurface.cpp
│   │   ├── tst_LoginSurface.cpp
│   │   ├── tst_SyncEngine.cpp
│   │   └── fixtures/                      ← sample *rc files
│   └── qml/                               ← qmltestrunner
│       └── tst_SurfaceCard.qml
├── po/                                    ← translations (auto-fed by l10n.kde.org once accepted)
└── docs/
    ├── architecture.md
    ├── plasmalogin-write.md               ← KAuth helper + future flatpak-spawn path
    ├── kde-store-submission.md
    └── development.md
```

## Layering rule (load-bearing — invariant for both future evolutions)

Four directional rules. Violating any of them re-couples the layers we
want to keep swappable.

1. **`src/core/` is GUI-free, Kirigami-free, KAuth-free, DBus-free.**
   It may depend only on QtCore + KConfig. It exposes plain Qt
   value types and `QObject` signals; it does not call
   `QGuiApplication`, never instantiates `QQuickItem`, never imports
   `KAuth/Action`, never opens a DBus connection. This is what makes a
   future KCM port a few-day job instead of a rewrite.
2. **Privileged writes go through `PrivilegedWriter` only.** `LoginSurface`
   knows it needs to write to `/etc/plasmalogin.conf` but doesn't know
   *how*. The concrete `KAuthPrivilegedWriter` lives under
   `src/privileged/kauth/`; the future `FlatpakSpawnPrivilegedWriter`
   lives next to it. The core gets one injected at construction and
   doesn't care which.
3. **DBus / Plasma-reload code lives only in `src/reload/`.** `core/`
   never touches `QDBusConnection`. The shell wires `PlasmaReloader` to
   the `SyncEngine`'s "applied" signal.
4. **`src/qml/core/` cannot `import org.kde.*`.** Same rule as
   ring-monitor's plasma-isolation seam. Kirigami imports go in
   `src/qml/platforms/plasma/` only; the core consumes theme tokens
   through explicit properties on an injected `theme: var` prop.

A reviewer should fail any PR that breaks one of these. A CI check
that greps `core/` for forbidden includes/imports is on the v1
checklist.

## Working rules (cross-cutting)

These apply everywhere in the repo regardless of which layer you're
editing.

- **English-only repo.** All committed files — code, comments,
  `docs/*.md`, every `CLAUDE.md`, `README.md`, commit messages, PR
  titles/bodies — are written exclusively in English. `qsTr()` /
  `i18n()` source strings stay English; translation is a downstream
  l10n.kde.org concern. Conversations with the user can be in any
  language; only what lands in the repo is constrained.
- **No nested ternaries.** `a ? x : b ? y : c ? z : d` → use a lookup
  map, a `switch`, or extract a named function. Single ternaries are
  OK.
- **500 lines max per source / test file** under `src/**/*.{cpp,h}`,
  `src/qml/**/*.qml`, and `tests/**`. When a file outgrows it: split.
  Extract a class into its own pair of files, or pull a sub-component
  into its own `.qml`. Don't raise the cap. `docs/*.md` and every
  `CLAUDE.md` are intentionally not capped.
- **KF6 / Qt docs before inventing.** Use KConfig accessors, not a
  hand-rolled INI parser. Use KAuth's helper pattern, not raw polkit
  DBus calls. Use `KIO::PreviewJob` for thumbnails, not a custom
  scaler. Check [api.kde.org](https://api.kde.org/frameworks/) and
  [doc.qt.io](https://doc.qt.io/qt-6/) before writing utility code —
  if KDE has the helper, use it.
- **All logic must be tested.** New core class ⇒ matching
  `tests/unit/tst_<Name>.cpp` with QTest. New QML view with public
  surface ⇒ `tests/qml/tst_<Name>.qml`. Fixture-driven tests for the
  `*rc` parsers — keep sample files under `tests/unit/fixtures/`.
- **REUSE / SPDX headers on every source file.** `SPDX-License-Identifier:
  GPL-2.0-or-later` + `SPDX-FileCopyrightText: <year> Manu <manu.acl@gmail.com>`.
  Required for KDE Review acceptance.
- **KDE Frameworks coding style** for C++ (4-space indent, brace on
  next line for functions, `m_` prefix for member variables). Enforced
  by `.clang-format` once scaffolded.
- **Conventional commits.** `feat:`, `fix:`, `chore:`, `docs:`, `ci:`,
  `refactor:`. Aligns with the global convention and unlocks
  `bump-label` later if a release pipeline is wired up.

## Design principles

### C++ core (standard SOLID)

| Letter | Concrete here |
|---|---|
| **S** Single Responsibility | `DesktopSurface` reads/writes desktop config and nothing else. `SyncEngine` orchestrates surfaces, doesn't parse INI files. |
| **O** Open/Closed | New surface types extend `WallpaperSurface`; new privileged-write mechanisms extend `PrivilegedWriter`. Adding the Flatpak path doesn't touch core. |
| **L** Liskov | All `WallpaperSurface` subclasses must honor the contract: `currentImage()` is cheap, `proposeImage()` is dry-run, `apply()` may fail and signal an error. |
| **I** Interface Segregation | `PrivilegedWriter` exposes one method, not a kitchen sink. `WallpaperSurface` does not have a `reloadPlasma()` slot — that's `PlasmaReloader`'s job. |
| **D** Dependency Inversion | `LoginSurface` takes a `PrivilegedWriter*` in its constructor; `SyncEngine` takes a list of `WallpaperSurface*`. Nothing in core constructs its dependencies — the shell does. |

### QML views (ring-monitor's adaptation)

QML has no nominal inheritance, so the SOLID grid rewrites:
**stateless components, data via props, events via signals, parents
wire them together.**

- One `.qml` file = one role. Logic in `.js` or in the C++ core, not
  in views.
- Leaf components don't reach into globals (no `Kirigami.Theme.X`
  directly in `core/` — go through the injected `theme` prop).
- Public surface = minimal props + signals. Test hooks `_`-prefixed.

Smells to flag during review:
- A core class `#include`s `<QQuickItem>` or `<KAuth/...>` → layering
  violation.
- A `core/*.qml` file has `import org.kde.kirigami` → plasma-isolation
  violation.
- A surface class opens a DBus connection → reload-isolation
  violation.
- An INI parser implemented by hand → ignored KConfig.

## App id

`dev.manuacl.plasmawallpapersync` for v1 (consistent with
`dev.manuacl.ringmonitor`).

If the project is accepted into KDE proper after Review Board, the id
becomes `org.kde.plasmawallpapersync` — anticipate this by keeping the
id in **one** CMake variable, not hardcoded everywhere.

## Where the rest will live

- **Layer-scoped rules** — once `src/core/`, `src/privileged/`,
  `src/qml/core/` and `tests/` have content, each will get its own
  `CLAUDE.md` with the scoped gotchas (KConfig pitfalls, KAuth helper
  registration, QML import quirks). This file stays the cross-cutting
  briefing.
- **Long-form rationale** — `docs/*.md` once written. The `CLAUDE.md`
  family is rule-shaped ("don't do X"); `docs/` is
  explanation-shaped ("the trade-off was…").
