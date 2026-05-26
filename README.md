# plasma-wallpaper-sync

GUI tool to view and edit, **independently or together**, the four background
images KDE Plasma 6 stores in separate configs — desktop, lock screen, login
screen, and session splash screen. Plasma's built-in UI splits these across
several settings panels with no shared view; this tool brings them into one
place.

## Motivation

Out of the box, Plasma 6 keeps four separate background configurations:

| Surface | Config file | Editable from |
|---|---|---|
| Desktop | `~/.config/plasma-org.kde.plasma.desktop-appletsrc` | Right-click desktop → *Configurer le bureau* |
| Lock screen (on resume / lock) | `~/.config/kscreenlockerrc` | Paramètres → *Verrouillage d'écran* → *Apparence* |
| Login screen (cold boot, plasmalogin) | `/etc/plasmalogin.conf` | Paramètres → *Écran de connexion* (or root edit) |
| Splash screen (session startup animation) | `~/.config/ksplashrc` + theme package | Paramètres → *Écran de démarrage* |

Note on the splash screen: it's technically a **Plasma "Look and Feel" theme
package**, not a single image — its background image lives inside the theme's
QML. Treating it like the other three means either patching a chosen theme's
image asset in place, or generating a small custom theme on the fly. To be
decided in design.

There is no built-in "apply to all" checkbox. The closest existing tool
([JoCode80/PlasmaWallpaperManager](https://github.com/JoCode80/PlasmaWallpaperManager))
targets **SDDM**, not **plasmalogin** — so it does not cover the login screen
on a Bazzite / modern Plasma 6 setup where plasmalogin is the display manager.

## Goal

A small GUI that:

1. Shows the **current** wallpaper for each of the three surfaces side by side
   (so you immediately see what mismatches what).
2. Lets the user pick a new image — or wallpaper pack / slideshow folder — and
   assign it to:
   - **one specific surface** (e.g. just the login screen), or
   - **any subset** (e.g. desktop + lockscreen, leave login & splash alone), or
   - **all four at once** (the "sync everything" shortcut).
3. Previews each choice before applying.
4. Triggers KDE to reload changed surfaces (no logout needed for desktop +
   lockscreen; greeter picks it up on next login).
5. Handles the privileged write to `/etc/plasmalogin.conf` via `pkexec` /
   polkit, not by running the whole app as root.

## Target environment

- KDE Plasma 6 on Wayland
- Display manager: **plasmalogin** (the new Plasma 6 greeter — not SDDM)
- Tested on Bazzite (Fedora atomic) but should work on any Plasma 6 distro
  with plasmalogin

## Status

Skeleton only. To be discussed in the next conversation:

- Stack: PyQt6 (like `astro-a50/gui/`) vs Qt6 + QML (like FretMind) — TBD
- Packaging: AppImage? Flatpak? rpm-ostree-friendly path?
- Scope: single image vs slideshow pack support
- Whether to also cover splashscreen (4th surface, less critical)

## Out of scope (for v1)

- SDDM support (different config format, not needed on this setup)
- Per-screen wallpaper differentiation
- Wallpaper plugins beyond `org.kde.image` (no Pictures-of-the-Day, no slideshow logic)
- Custom splash screen theme authoring (only image swap inside an existing theme)
