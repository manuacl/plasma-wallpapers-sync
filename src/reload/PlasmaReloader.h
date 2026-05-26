// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PWS_RELOAD_PLASMA_RELOADER_H
#define PWS_RELOAD_PLASMA_RELOADER_H

#include <QObject>
#include <QString>

/**
 * Sole entry point for D-Bus traffic in this project.
 *
 * CLAUDE.md confines QtDBus to src/reload/ so the layering rule
 * "core/ never opens a DBus connection" stays mechanically enforced
 * by the CMake layering guard. The shell instantiates one
 * PlasmaReloader and connects it to SyncEngine::surfaceApplySucceeded
 * — when a surface reports success, the corresponding notifyXxx slot
 * fires and Plasma is asked to re-read its wallpaper config.
 *
 * Per-surface behavior:
 *   - Desktop  → org.kde.PlasmaShell.evaluateScript with a one-liner
 *     that forces each desktop containment to re-evaluate its
 *     wallpaper plugin's config. KConfigWatcher catches the file
 *     change but does not re-render the image on its own.
 *   - Lockscreen → no-op in v1. kscreenlocker re-reads its config
 *     on the next lock event; we'll add a D-Bus poke here if user
 *     feedback shows the new image lags.
 *   - Login → no-op by design. The plasmalogin greeter runs only at
 *     boot/logout; the new wallpaper appears at the next login.
 *
 * Failures are reported through notificationFailed rather than
 * thrown — the apply itself succeeded, this is best-effort polish.
 */
class PlasmaReloader : public QObject
{
    Q_OBJECT
public:
    explicit PlasmaReloader(QObject *parent = nullptr);
    ~PlasmaReloader() override;

public Q_SLOTS:
    void notifyDesktopChanged();
    void notifyLockscreenChanged();
    void notifyLoginChanged();

Q_SIGNALS:
    void notificationFailed(const QString &surface, const QString &reason);
};

#endif // PWS_RELOAD_PLASMA_RELOADER_H
