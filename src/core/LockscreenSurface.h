// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PWS_CORE_LOCKSCREEN_SURFACE_H
#define PWS_CORE_LOCKSCREEN_SURFACE_H

#include "WallpaperSurface.h"

/**
 * Lock-screen wallpaper surface.
 *
 * Reads and writes the `Image` entry under
 * `~/.config/kscreenlockerrc`'s `[Greeter][Wallpaper][org.kde.image][General]`
 * group. Unlike the desktop config there are no per-screen
 * containments to disambiguate — the group path is fixed — so the
 * implementation only needs to honor the well-known location.
 *
 * Plugins other than `org.kde.image` are out of scope (see README).
 *
 * The constructor accepts an explicit config path to keep the class
 * trivially testable against fixtures.
 */
class LockscreenSurface : public WallpaperSurface
{
    Q_OBJECT
public:
    explicit LockscreenSurface(QObject *parent = nullptr);
    explicit LockscreenSurface(const QString &configPath, QObject *parent = nullptr);
    ~LockscreenSurface() override;

    QString id() const override;
    QString displayName() const override;
    QString description() const override;
    QString currentImagePath() const override;

public Q_SLOTS:
    void apply(const QString &imagePath) override;

private:
    static QString defaultConfigPath();

    QString m_configPath;
};

#endif // PWS_CORE_LOCKSCREEN_SURFACE_H
