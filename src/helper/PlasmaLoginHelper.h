// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PWS_HELPER_PLASMA_LOGIN_HELPER_H
#define PWS_HELPER_PLASMA_LOGIN_HELPER_H

#include <KAuth/ActionReply>
#include <KAuth/HelperSupport>

#include <QObject>
#include <QVariantMap>

/**
 * Privileged helper invoked over D-Bus by KAuthPrivilegedWriter when
 * the GUI applies a wallpaper to the login surface.
 *
 * The helper runs as root for the duration of a single action and
 * exits — polkit handles authentication via the action id declared
 * in data/dev.manuacl.plasmawallpapersync.policy
 * (allow_active=auth_admin, so the user sees a sudo prompt the
 * first time per session).
 *
 * Security posture: the helper accepts only an atomic-write request
 * to a single hard-coded path (/etc/plasmalogin.conf). It refuses
 * any other path explicitly rather than relying on polkit gating —
 * defense in depth in case a misconfiguration ever weakens the
 * polkit defaults.
 */
class PlasmaLoginHelper : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    /**
     * Action method exposed to KAuth::Action as
     * "dev.manuacl.plasmawallpapersync.writefile".
     *
     * Arguments (QVariantMap):
     *   - "path": QString — must be exactly "/etc/plasmalogin.conf"
     *   - "contents": QByteArray — bytes to write
     *
     * Returns KAuth::ActionReply::SuccessReply() on success, a
     * HelperErrorReply with an errorDescription otherwise.
     */
    KAuth::ActionReply writefile(const QVariantMap &args);
};

#endif // PWS_HELPER_PLASMA_LOGIN_HELPER_H
