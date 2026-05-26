// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PWS_CORE_DESKTOP_SURFACE_H
#define PWS_CORE_DESKTOP_SURFACE_H

#include "WallpaperSurface.h"

#include <QStringList>

/**
 * Desktop wallpaper surface.
 *
 * Reads and writes the `Image` entry of the first containment whose
 * plugin is `org.kde.desktopcontainment` inside
 * `~/.config/plasma-org.kde.plasma.desktop-appletsrc`. Per-screen
 * differentiation is out of scope for v1 (see README); this class
 * treats the first matching containment as the primary one.
 *
 * The constructor accepts an explicit config path to make the class
 * trivially testable against fixtures with QTEST_GUILESS_MAIN.
 */
class DesktopSurface : public WallpaperSurface
{
    Q_OBJECT
public:
    explicit DesktopSurface(QObject *parent = nullptr);
    explicit DesktopSurface(const QString &configPath, QObject *parent = nullptr);
    ~DesktopSurface() override;

    QString id() const override;
    QString displayName() const override;
    QString description() const override;
    QString currentImagePath() const override;

public Q_SLOTS:
    void apply(const QString &imagePath) override;

private:
    QString findPrimaryContainmentId() const;
    QStringList findAllDesktopContainmentIds() const;
    static QString defaultConfigPath();

    QString m_configPath;
};

#endif // PWS_CORE_DESKTOP_SURFACE_H
