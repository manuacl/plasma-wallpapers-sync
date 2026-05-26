#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
# SPDX-License-Identifier: CC0-1.0
#
# Native KAuth install for end-to-end testing on a Bazzite (or any
# rpm-ostree-atomic) host.
#
# The Flatpak runtime ships the KAuth helper-side plugin but NOT the
# client-side polkit backend (/usr/lib/plugins/kf6/kauth/backend/ is
# missing), so a Flatpak install of this app can never reach polkit.
# End-to-end KAuth testing therefore requires a native binary (built
# in dev/toolbox or shipped via RPM/AUR) plus this script's host
# install of the supporting files:
#
#   /usr/local/libexec/plasma-wallpaper-sync-helper
#       The privileged helper. Host's KF6 matches the runtime's
#       (6.26.x on Bazzite Plasma 6.10), so a binary built in a
#       fedora-toolbox is ABI-compatible.
#
#   /etc/polkit-1/actions/dev.manuacl.plasmawallpapersync.policy
#       Polkit action policy. Fedora-based polkit reads this dir in
#       addition to /usr/share/polkit-1/actions.
#
#   /etc/dbus-1/system-services/dev.manuacl.plasmawallpapersync.service
#       D-Bus system-bus activation file. The Exec= line is patched
#       to point at the host helper path above.
#
#   /etc/dbus-1/system.d/95-plasma-wallpaper-sync.conf
#       D-Bus config that
#         (a) adds /etc/dbus-1/system-services to the daemon's
#             servicedir list (the compiled-in defaults only cover
#             /usr/share/dbus-1/system-services, which is r/o here),
#         (b) restates the helper bus-name policy verbatim
#             ("only root owns the name, anyone can send to it").
#
# Uninstall: `sudo rm -f` those four paths.
#
# Production install (when shipping v0.1.0 as RPM/AUR) installs under
# the canonical /usr/ paths via `cmake --install` — see
# src/helper/CMakeLists.txt, which guards the system-path installs
# behind a Flatpak detection.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$REPO_ROOT"

# Locate the most recent build state — prefer a native toolbox build
# (build-native/) since the Flatpak build state lives behind the
# Flatpak's namespaced lib paths.
if [[ -f "build-native/bin/plasma-wallpaper-sync-helper" ]]; then
    HELPER_SRC="build-native/bin/plasma-wallpaper-sync-helper"
elif [[ -f "build-native/src/helper/plasma-wallpaper-sync-helper" ]]; then
    HELPER_SRC="build-native/src/helper/plasma-wallpaper-sync-helper"
else
    BUILD_ROOT=$(ls -td .flatpak-builder/build/plasma-wallpaper-sync-*/ 2>/dev/null | head -1)
    if [[ -z "$BUILD_ROOT" ]]; then
        echo "FAIL: no native or Flatpak build found. Run a toolbox build first." >&2
        exit 1
    fi
    HELPER_SRC="${BUILD_ROOT%/}/_flatpak_build/bin/plasma-wallpaper-sync-helper"
fi

POLICY_SRC="data/dev.manuacl.plasmawallpapersync.policy"

for f in "$HELPER_SRC" "$POLICY_SRC"; do
    [[ -f "$f" ]] || { echo "FAIL: missing $f" >&2; exit 1; }
done

HELPER_DST=/usr/local/libexec/plasma-wallpaper-sync-helper
POLICY_DST=/etc/polkit-1/actions/dev.manuacl.plasmawallpapersync.policy
SVC_DST=/etc/dbus-1/system-services/dev.manuacl.plasmawallpapersync.service
CONF_DST=/etc/dbus-1/system.d/95-plasma-wallpaper-sync.conf

# Generate the .service and .conf from scratch — the Flatpak build's
# copies bake in an Exec path under /app/, and the previous attempt
# to splice in a <servicedir> via sed produced an invalid file with
# nested <busconfig> elements. Inline templates remove that class of
# bug.
PATCHED_SVC=$(mktemp)
PATCHED_CONF=$(mktemp)
trap 'rm -f "$PATCHED_SVC" "$PATCHED_CONF"' EXIT

cat > "$PATCHED_SVC" <<EOF
[D-BUS Service]
Name=dev.manuacl.plasmawallpapersync
Exec=$HELPER_DST
User=root
EOF

cat > "$PATCHED_CONF" <<'EOF'
<!DOCTYPE busconfig PUBLIC
 "-//freedesktop//DTD D-BUS Bus Configuration 1.0//EN"
 "http://www.freedesktop.org/standards/dbus/1.0/busconfig.dtd">
<busconfig>
  <servicedir>/etc/dbus-1/system-services</servicedir>

  <policy user="root">
    <allow own="dev.manuacl.plasmawallpapersync"/>
  </policy>

  <policy context="default">
    <allow send_destination="dev.manuacl.plasmawallpapersync"/>
  </policy>
</busconfig>
EOF

echo "Installing:"
echo "  $HELPER_DST"
echo "  $POLICY_DST"
echo "  $SVC_DST       (Exec=$HELPER_DST)"
echo "  $CONF_DST"
echo ""

sudo -A install -D -m 0755 "$HELPER_SRC"    "$HELPER_DST"
sudo -A install -D -m 0644 "$POLICY_SRC"    "$POLICY_DST"
sudo -A install -D -m 0644 "$PATCHED_SVC"   "$SVC_DST"
sudo -A install -D -m 0644 "$PATCHED_CONF"  "$CONF_DST"

echo ""
echo "Reloading D-Bus config + polkit so they pick up the new files…"
dbus-send --system --print-reply --type=method_call \
    --dest=org.freedesktop.DBus / org.freedesktop.DBus.ReloadConfig \
    >/dev/null 2>&1 \
    && echo "  D-Bus ReloadConfig: ok" \
    || echo "  D-Bus ReloadConfig: failed — file may still be malformed"
sudo -A systemctl reload polkit.service 2>/dev/null \
    || echo "  polkit: auto-reloads on policy file change, nothing to do"

echo ""
echo "Verifying with pkaction:"
if pkaction --action-id dev.manuacl.plasmawallpapersync.writefile >/dev/null 2>&1; then
    echo "  polkit recognizes the action ok"
else
    echo "  polkit DOES NOT recognize the action — re-login or reboot may be needed"
fi

echo ""
echo "Verifying D-Bus activation:"
if dbus-send --system --print-reply --dest=dev.manuacl.plasmawallpapersync \
        / org.freedesktop.DBus.Introspectable.Introspect \
        >/dev/null 2>&1; then
    echo "  D-Bus activated the helper ok"
else
    echo "  D-Bus could NOT activate the service — check the .conf XML and re-run"
fi

echo ""
echo "Done. Uninstall:"
echo "  sudo rm -f $HELPER_DST $POLICY_DST $SVC_DST $CONF_DST"
