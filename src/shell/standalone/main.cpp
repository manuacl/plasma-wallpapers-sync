// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DesktopSurface.h"
#include "LockscreenSurface.h"
#include "SyncEngine.h"

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("manuacl"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("manuacl.dev"));
    QCoreApplication::setApplicationName(QStringLiteral("plasma-wallpaper-sync"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QGuiApplication::setApplicationDisplayName(
        QGuiApplication::translate("Main", "Plasma Wallpaper Sync"));
    QIcon::setFallbackThemeName(QStringLiteral("breeze"));

    // LoginSurface is held back until the KAuth helper lands — the
    // shell would have to inject a PrivilegedWriter, and we don't
    // want to ship the GUI half-wired. Desktop + Lockscreen are
    // fully functional today.
    SyncEngine engine;
    DesktopSurface desktop;
    LockscreenSurface lockscreen;
    engine.addSurface(&desktop);
    engine.addSurface(&lockscreen);

    QQmlApplicationEngine qmlEngine;
    qmlEngine.rootContext()->setContextProperty(QStringLiteral("syncEngine"), &engine);
    qmlEngine.loadFromModule(QStringLiteral("PlasmaWallpaperSync"),
                              QStringLiteral("Main"));
    if (qmlEngine.rootObjects().isEmpty()) {
        return 1;
    }

    return app.exec();
}
