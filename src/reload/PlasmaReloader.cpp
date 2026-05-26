// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PlasmaReloader.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

namespace
{
const QString kDefaultPlasmaShellService = QStringLiteral("org.kde.plasmashell");
const QString kPlasmaShellPath = QStringLiteral("/PlasmaShell");
const QString kPlasmaShellIface = QStringLiteral("org.kde.PlasmaShell");
const QString kDesktopSurface = QStringLiteral("desktop");

// Canonical Plasma 6 wallpaper-set script. Three earlier variants
// silently no-op'd against the live plugin:
//   - `d.wallpaperPlugin = d.wallpaperPlugin` — same-value setter is
//     short-circuited by Plasma
//   - `d.reloadConfig()` — recognized but the wallpaper plugin
//     doesn't react to it (it caches its config in memory and only
//     re-reads it when explicitly poked through writeConfig)
//   - `wallpaperPlugin = 'org.kde.color'; wallpaperPlugin = 'org.kde.image'`
//     — both setters happen in the same script evaluation, Plasma
//     batches them into a "net zero change" and skips the reload
//
// The only reliable reload trigger is the same call System Settings
// makes when the user clicks Apply in its Wallpaper page: write the
// Image value through Plasma's containment scripting. That goes
// through the wallpaper plugin's setter path, which emits the
// repaint signal that the renderer actually listens for. The fact
// that we also wrote the same value to disk via KConfig moments
// earlier is what keeps the file authoritative across restarts;
// this call is the live-update bridge.
const QString kDesktopReloadScriptTemplate = QStringLiteral(
    "desktops().forEach(function(d) { "
    "d.currentConfigGroup = ['Wallpaper', 'org.kde.image', 'General']; "
    "d.writeConfig('Image', '%1'); "
    "});");

QString escapeForSingleQuotedJsString(const QString &s)
{
    QString out = s;
    out.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    out.replace(QLatin1Char('\''), QStringLiteral("\\'"));
    return out;
}
}

PlasmaReloader::PlasmaReloader(QObject *parent)
    : PlasmaReloader(kDefaultPlasmaShellService, parent)
{
}

PlasmaReloader::PlasmaReloader(const QString &plasmashellService, QObject *parent)
    : QObject(parent)
    , m_plasmashellService(plasmashellService)
{
}

PlasmaReloader::~PlasmaReloader() = default;

void PlasmaReloader::notifyDesktopChanged(const QString &imagePath)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        Q_EMIT notificationFailed(kDesktopSurface, tr("No D-Bus session bus available"));
        return;
    }

    QDBusInterface iface(m_plasmashellService, kPlasmaShellPath, kPlasmaShellIface, bus);
    if (!iface.isValid()) {
        Q_EMIT notificationFailed(kDesktopSurface, iface.lastError().message());
        return;
    }

    const QString script = kDesktopReloadScriptTemplate.arg(
        escapeForSingleQuotedJsString(imagePath));
    QDBusReply<QString> reply = iface.call(QStringLiteral("evaluateScript"), script);
    if (!reply.isValid()) {
        Q_EMIT notificationFailed(kDesktopSurface, reply.error().message());
    }
}

void PlasmaReloader::notifyLockscreenChanged()
{
    // Intentional no-op — see header.
}

void PlasmaReloader::notifyLoginChanged()
{
    // Intentional no-op — see header.
}
