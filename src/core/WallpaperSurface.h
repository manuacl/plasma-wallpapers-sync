// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PWS_CORE_WALLPAPER_SURFACE_H
#define PWS_CORE_WALLPAPER_SURFACE_H

#include <QObject>
#include <QString>

/**
 * Abstract base for one of the four Plasma 6 wallpaper surfaces
 * (desktop, lockscreen, login, splash). Subclasses encapsulate the
 * "where does the current image path live, and how do I rewrite it"
 * for one specific surface.
 *
 * Privileged writes (login screen) are delegated through an injected
 * PrivilegedWriter so the same surface class survives a future
 * Flatpak-portal-based mechanism without modification. See CLAUDE.md
 * for the layering rule that makes this isolation load-bearing.
 */
class WallpaperSurface : public QObject
{
    Q_OBJECT
public:
    explicit WallpaperSurface(QObject *parent = nullptr);
    ~WallpaperSurface() override;

    /** Stable, untranslated identifier ("desktop", "lockscreen", ...). */
    virtual QString id() const = 0;

    /** Translated, human-facing label shown in the GUI card. */
    virtual QString displayName() const = 0;

    /** Current image path or URL as recorded by the live system. */
    virtual QString currentImagePath() const = 0;

public Q_SLOTS:
    /** Write the new image. Always emits exactly one of the two
     *  outcome signals; may also emit currentImagePathChanged. */
    virtual void apply(const QString &imagePath) = 0;

Q_SIGNALS:
    void currentImagePathChanged();
    void applySucceeded();
    void applyFailed(const QString &reason);
};

#endif // PWS_CORE_WALLPAPER_SURFACE_H
