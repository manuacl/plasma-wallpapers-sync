// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PWS_CORE_LOGIN_SURFACE_H
#define PWS_CORE_LOGIN_SURFACE_H

#include "WallpaperSurface.h"

class PrivilegedWriter;

/**
 * Login-screen (plasmalogin) wallpaper surface.
 *
 * Reads `/etc/plasmalogin.conf` directly (the file is world-readable,
 * no privilege needed) and writes through an injected PrivilegedWriter
 * — KAuth-based today, possibly flatpak-spawn-based in a Flatpak
 * distribution. The surface itself stays unaware of how privilege is
 * obtained, which is the entire point of the indirection.
 *
 * The on-disk format matches kscreenlockerrc exactly:
 * `[Greeter][Wallpaper][org.kde.image][General] Image=...`.
 *
 * Apply strategy: a two-step pipeline through the PrivilegedWriter.
 *
 *   1. installFile() copies the user-chosen image into
 *      /var/lib/plasmalogin/wallpapers/<basename>. The greeter runs
 *      as the `plasmalogin` system user, which has no access to $HOME
 *      (mode drwx------); pointing the conf at /home/.../foo.jpg
 *      directly causes a silent fallback to the previous wallpaper.
 *   2. writeAtomically() replaces /etc/plasmalogin.conf with a
 *      version whose Image= key points at the just-installed copy.
 *
 * Step 2 only runs if step 1 succeeded. Both steps are gated by polkit
 * (auth_admin), so the user may see two prompts the first time per
 * session. The KConfig mutation that produces step 2's bytes happens
 * in unprivileged user space — the helper never parses KConfig.
 */
class LoginSurface : public WallpaperSurface
{
    Q_OBJECT
public:
    /** Production constructor — read and write paths default to
     *  Flatpak-aware values (see defaultReadPath / defaultWritePath). */
    explicit LoginSurface(PrivilegedWriter *writer, QObject *parent = nullptr);

    /** Single-path constructor: both read and write target the same
     *  file. Convenient for tests where a temp file plays both roles. */
    explicit LoginSurface(PrivilegedWriter *writer,
                          const QString &configPath,
                          QObject *parent = nullptr);

    /** Explicit read/write decoupling for the Flatpak case: the
     *  sandbox exposes the host's /etc/plasmalogin.conf at
     *  /run/host/etc/plasmalogin.conf for reading, but the privileged
     *  helper runs on the host and validates the path it writes to
     *  against "/etc/plasmalogin.conf". */
    explicit LoginSurface(PrivilegedWriter *writer,
                          const QString &readPath,
                          const QString &writePath,
                          QObject *parent = nullptr);

    ~LoginSurface() override;

    QString id() const override;
    QString displayName() const override;
    QString currentImagePath() const override;
    QString previewImagePath() const override;

    /** Tests inject a fake cache location to keep $XDG_CACHE_HOME
     *  untouched. Empty string ⇒ use QStandardPaths::CacheLocation. */
    void setPreviewSourceCachePath(const QString &path);

public Q_SLOTS:
    void apply(const QString &imagePath) override;

private:
    void wireWriterSignals();
    void writeConfWithImage(const QString &installedImageUrl);
    void persistPreviewSourceMapping(const QString &installedDest,
                                     const QString &sourceUrl);
    QString readCachedSourceUrlFor(const QString &installedDest) const;
    QString previewSourceCachePath() const;
    static QString defaultReadPath();
    static QString defaultWritePath();
    static QString sanitizeBasename(const QString &candidate);
    static QString resolveLocalPath(const QString &maybeUrl);

    PrivilegedWriter *m_writer; // not owned
    QString m_readPath;
    QString m_writePath;
    QString m_pendingInstallDest;
    QString m_pendingSourceUrl;
    QString m_overrideCachePath;
};

#endif // PWS_CORE_LOGIN_SURFACE_H
