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

// "Re-set the same wallpaperPlugin" is a load-bearing no-op assignment:
// Plasma's containment scripting treats the setter as a config change
// trigger and re-reads the underlying plugin group from disk, picking
// up whatever DesktopSurface just wrote.
const QString kDesktopReloadScript = QStringLiteral(
    "desktops().forEach(function(d) { d.wallpaperPlugin = d.wallpaperPlugin; });");
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

void PlasmaReloader::notifyDesktopChanged()
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

    QDBusReply<void> reply = iface.call(QStringLiteral("evaluateScript"), kDesktopReloadScript);
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
