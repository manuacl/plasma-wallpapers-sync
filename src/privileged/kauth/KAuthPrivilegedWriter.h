// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PWS_PRIVILEGED_KAUTH_KAUTH_PRIVILEGED_WRITER_H
#define PWS_PRIVILEGED_KAUTH_KAUTH_PRIVILEGED_WRITER_H

#include "PrivilegedWriter.h"

/**
 * KAuth-backed concrete PrivilegedWriter: dispatches each
 * writeAtomically() call to the helper binary at
 * /usr/libexec/.../plasma-wallpaper-sync-helper through KAuth::Action,
 * which polkit-gates against the action id declared in
 * data/dev.manuacl.plasmawallpapersync.policy.
 *
 * The helper is the only piece running as root. Polkit prompts the
 * user once per session (auth_admin); after that the helper is
 * invoked transparently. KAuth handles all the D-Bus mechanics —
 * we just shape the QVariantMap and forward the result signal.
 *
 * This file lives under src/privileged/kauth/ rather than src/core/
 * because it links KF6::AuthCore, which is on the core/ layering
 * guard's deny list. A future Flatpak-portal-based replacement
 * would live alongside as src/privileged/flatpak/ and provide an
 * alternative concrete PrivilegedWriter without touching core/.
 */
class KAuthPrivilegedWriter : public PrivilegedWriter
{
    Q_OBJECT
public:
    explicit KAuthPrivilegedWriter(QObject *parent = nullptr);
    ~KAuthPrivilegedWriter() override;

public Q_SLOTS:
    void writeAtomically(const QString &path, const QByteArray &contents) override;
    void installFile(const QString &srcPath, const QString &destPath) override;
};

#endif // PWS_PRIVILEGED_KAUTH_KAUTH_PRIVILEGED_WRITER_H
