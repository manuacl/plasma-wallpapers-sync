// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PWS_CORE_PRIVILEGED_WRITER_H
#define PWS_CORE_PRIVILEGED_WRITER_H

#include <QByteArray>
#include <QObject>
#include <QString>

/**
 * Abstract interface for "write these bytes to this path, with
 * whatever elevation the host environment requires".
 *
 * Concrete implementations live in `src/privileged/`:
 *   - KAuthPrivilegedWriter: KAuth helper + polkit policy (v1, native install)
 *   - FlatpakSpawnPrivilegedWriter: flatpak-spawn --host pkexec (v3, Flathub)
 *
 * Surfaces that touch user-owned config files (DesktopSurface,
 * LockscreenSurface) bypass this entirely and use KConfig directly —
 * privilege only enters the picture for /etc/plasmalogin.conf.
 *
 * Contract:
 *   - writeAtomically replaces the file at `path` with `contents`,
 *     preserving permissions and ownership. Concrete implementations
 *     SHOULD write to a sibling tempfile and rename(), not truncate.
 *   - installFile copies the file at `srcPath` to `destPath` with
 *     permissions readable by the plasmalogin system user (mode 0644,
 *     ownership plasmalogin:plasmalogin). LoginSurface uses this to
 *     stage a wallpaper into /var/lib/plasmalogin/wallpapers/ before
 *     pointing /etc/plasmalogin.conf at the staged copy — the greeter
 *     runs as the `plasmalogin` user and has no access to $HOME.
 *   - Always emits exactly one of {write,install}Succeeded /
 *     {write,install}Failed per call, keyed by the destination path
 *     so a single writer can serve several surfaces unambiguously.
 *   - Operation may be asynchronous (KAuth dispatches through D-Bus);
 *     callers must rely on the signals, not on the call returning.
 */
class PrivilegedWriter : public QObject
{
    Q_OBJECT
public:
    explicit PrivilegedWriter(QObject *parent = nullptr);
    ~PrivilegedWriter() override;

public Q_SLOTS:
    virtual void writeAtomically(const QString &path, const QByteArray &contents) = 0;
    virtual void installFile(const QString &srcPath, const QString &destPath) = 0;

Q_SIGNALS:
    void writeSucceeded(const QString &path);
    void writeFailed(const QString &path, const QString &reason);
    void installSucceeded(const QString &destPath);
    void installFailed(const QString &destPath, const QString &reason);
};

#endif // PWS_CORE_PRIVILEGED_WRITER_H
