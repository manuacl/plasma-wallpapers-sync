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
 * Apply strategy: stage a user-writable copy of the file, mutate it
 * with KConfig in unprivileged space, then hand the resulting bytes
 * to the writer. This keeps the privileged process minimal — it only
 * needs to know how to atomically replace a file, not how to parse
 * KConfig.
 */
class LoginSurface : public WallpaperSurface
{
    Q_OBJECT
public:
    explicit LoginSurface(PrivilegedWriter *writer, QObject *parent = nullptr);
    explicit LoginSurface(PrivilegedWriter *writer,
                          const QString &configPath,
                          QObject *parent = nullptr);
    ~LoginSurface() override;

    QString id() const override;
    QString displayName() const override;
    QString currentImagePath() const override;

public Q_SLOTS:
    void apply(const QString &imagePath) override;

private:
    void wireWriterSignals();
    static QString defaultConfigPath();

    PrivilegedWriter *m_writer; // not owned
    QString m_configPath;
};

#endif // PWS_CORE_LOGIN_SURFACE_H
