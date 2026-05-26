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
    Q_PROPERTY(QString id READ id CONSTANT)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(QString currentImagePath READ currentImagePath NOTIFY currentImagePathChanged)
    Q_PROPERTY(QString previewImagePath READ previewImagePath NOTIFY currentImagePathChanged)
public:
    explicit WallpaperSurface(QObject *parent = nullptr);
    ~WallpaperSurface() override;

    /** Stable, untranslated identifier ("desktop", "lockscreen", ...). */
    virtual QString id() const = 0;

    /** Translated, human-facing label shown in the GUI card. */
    virtual QString displayName() const = 0;

    /** Current image path or URL as recorded by the live system —
     *  the canonical truth for the consumer of the surface (Plasma
     *  shell, kscreenlocker, plasmalogin, …). */
    virtual QString currentImagePath() const = 0;

    /** Image URL the GUI should use for the on-card preview.
     *  Defaults to currentImagePath() — only LoginSurface overrides,
     *  because /var/lib/plasmalogin/wallpapers/ is not readable from
     *  the invoking user even though the conf path stored there is
     *  what plasmalogin actually consumes. The override returns the
     *  user-side source URL that produced the install, falling back
     *  to currentImagePath() when no source mapping is on record. */
    virtual QString previewImagePath() const { return currentImagePath(); }

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
