// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DesktopSurface.h"
#include "KAuthPrivilegedWriter.h"
#include "LockscreenSurface.h"
#include "LoginSurface.h"
#include "PlasmaReloader.h"
#include "SyncEngine.h"
#include "WallpaperLibrary.h"
#include "WallpaperSurface.h"

#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>

int main(int argc, char **argv)
{
    // Bypass the XDG FileChooser portal under Flatpak so FileDialog
    // returns real host paths ("file:///home/user/Pictures/foo.jpg")
    // instead of sandbox-scoped portal URLs
    // ("file:///run/user/1000/doc/<token>/foo.jpg"). The portal path
    // is read-only inside our sandbox; written back to a Plasma
    // config file, it's unreachable by the host kscreenlocker /
    // plasmashell processes, so the wallpaper never visibly updates.
    QCoreApplication::setAttribute(Qt::AA_DontUseNativeDialogs);

    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("manuacl"));
    QCoreApplication::setOrganizationDomain(QStringLiteral("manuacl.dev"));
    QCoreApplication::setApplicationName(QStringLiteral("plasma-wallpaper-sync"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));
    QGuiApplication::setApplicationDisplayName(
        QGuiApplication::translate("Main", "Plasma Wallpaper Sync"));
    QIcon::setFallbackThemeName(QStringLiteral("breeze"));

    SyncEngine engine;
    DesktopSurface desktop;
    LockscreenSurface lockscreen;
    KAuthPrivilegedWriter privilegedWriter;
    LoginSurface login(&privilegedWriter);
    engine.addSurface(&desktop);
    engine.addSurface(&lockscreen);
    engine.addSurface(&login);

    // Plasma doesn't re-render the desktop wallpaper from disk on
    // its own when we rewrite the appletsrc — KConfigWatcher catches
    // the file change but the wallpaper plugin doesn't act on it.
    // Poke org.kde.PlasmaShell via D-Bus on every successful apply.
    // Lockscreen and login are notify no-ops by design (kscreenlocker
    // re-reads on the next lock event; plasmalogin only runs at boot).
    PlasmaReloader reloader;
    QObject::connect(&engine, &SyncEngine::surfaceApplySucceeded,
                     &reloader, [&reloader, &engine](const QString &id) {
        WallpaperSurface *s = engine.surface(id);
        if (id == QStringLiteral("desktop")) {
            reloader.notifyDesktopChanged(s ? s->currentImagePath() : QString());
        } else if (id == QStringLiteral("lockscreen")) {
            reloader.notifyLockscreenChanged();
        } else if (id == QStringLiteral("login")) {
            reloader.notifyLoginChanged();
        }
    });

    WallpaperLibrary wallpapers;

    QQmlApplicationEngine qmlEngine;
    qmlEngine.rootContext()->setContextProperty(QStringLiteral("syncEngine"), &engine);
    qmlEngine.rootContext()->setContextProperty(QStringLiteral("wallpaperLibrary"), &wallpapers);
    qmlEngine.loadFromModule(QStringLiteral("PlasmaWallpaperSync"),
                              QStringLiteral("Main"));
    if (qmlEngine.rootObjects().isEmpty()) {
        return 1;
    }

    return app.exec();
}
