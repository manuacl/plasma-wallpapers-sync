// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PlasmaLoginHelper.h"

#include <KAuth/HelperSupport>

#include <QSaveFile>
#include <QString>

namespace
{
const QString kAllowedPath = QStringLiteral("/etc/plasmalogin.conf");
}

KAuth::ActionReply PlasmaLoginHelper::writefile(const QVariantMap &args)
{
    const QString path = args.value(QStringLiteral("path")).toString();
    const QByteArray contents = args.value(QStringLiteral("contents")).toByteArray();

    // Defense in depth — the polkit policy is the primary gate, but
    // we refuse any path other than /etc/plasmalogin.conf here too
    // so a misconfigured policy can never escalate this helper into
    // a "write anywhere as root" primitive.
    if (path != kAllowedPath) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(
            QStringLiteral("Refusing to write to %1; only %2 is permitted")
                .arg(path, kAllowedPath));
        return reply;
    }

    // QSaveFile writes to a sibling temp file and atomically renames
    // on commit() — keeps the original intact on any partial-write
    // failure.
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(file.errorString());
        return reply;
    }

    if (file.write(contents) != contents.size()) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(file.errorString());
        return reply;
    }

    if (!file.commit()) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(file.errorString());
        return reply;
    }

    return KAuth::ActionReply::SuccessReply();
}

KAUTH_HELPER_MAIN("dev.manuacl.plasmawallpapersync", PlasmaLoginHelper)
