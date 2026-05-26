// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "KAuthPrivilegedWriter.h"

#include <KAuth/Action>
#include <KAuth/ExecuteJob>

#include <QVariantMap>

namespace
{
const QString kHelperId = QStringLiteral("dev.manuacl.plasmawallpapersync");
const QString kActionName = QStringLiteral("dev.manuacl.plasmawallpapersync.writefile");
}

KAuthPrivilegedWriter::KAuthPrivilegedWriter(QObject *parent)
    : PrivilegedWriter(parent)
{
}

KAuthPrivilegedWriter::~KAuthPrivilegedWriter() = default;

void KAuthPrivilegedWriter::writeAtomically(const QString &path, const QByteArray &contents)
{
    KAuth::Action action(kActionName);
    action.setHelperId(kHelperId);

    QVariantMap args;
    args[QStringLiteral("path")] = path;
    args[QStringLiteral("contents")] = contents;
    action.setArguments(args);

    KAuth::ExecuteJob *job = action.execute();
    if (!job) {
        Q_EMIT writeFailed(path, tr("Could not start the privileged write action"));
        return;
    }

    // Connect BEFORE start() — a job that errors out immediately
    // would otherwise emit result before any listener is attached.
    connect(job, &KAuth::ExecuteJob::result, this, [this, path, job]() {
        if (job->error()) {
            Q_EMIT writeFailed(path, job->errorString());
        } else {
            Q_EMIT writeSucceeded(path);
        }
        job->deleteLater();
    });

    job->start();
}
