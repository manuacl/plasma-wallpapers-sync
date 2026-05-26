// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LoginSurface.h"

#include "PrivilegedWriter.h"

#include <KConfig>
#include <KConfigGroup>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QUrl>

namespace
{
const QString kGreeter = QStringLiteral("Greeter");
const QString kWallpaper = QStringLiteral("Wallpaper");
const QString kImagePlugin = QStringLiteral("org.kde.image");
const QString kGeneral = QStringLiteral("General");
const QString kImageKey = QStringLiteral("Image");
const QString kHostConfigPath = QStringLiteral("/etc/plasmalogin.conf");
const QString kFlatpakInfoMarker = QStringLiteral("/.flatpak-info");
const QString kFlatpakHostReadPath = QStringLiteral("/run/host/etc/plasmalogin.conf");
}

LoginSurface::LoginSurface(PrivilegedWriter *writer, QObject *parent)
    : LoginSurface(writer, defaultReadPath(), defaultWritePath(), parent)
{
}

LoginSurface::LoginSurface(PrivilegedWriter *writer, const QString &configPath, QObject *parent)
    : LoginSurface(writer, configPath, configPath, parent)
{
}

LoginSurface::LoginSurface(PrivilegedWriter *writer,
                            const QString &readPath,
                            const QString &writePath,
                            QObject *parent)
    : WallpaperSurface(parent)
    , m_writer(writer)
    , m_readPath(readPath)
    , m_writePath(writePath)
{
    wireWriterSignals();
}

LoginSurface::~LoginSurface() = default;

QString LoginSurface::id() const
{
    return QStringLiteral("login");
}

QString LoginSurface::displayName() const
{
    return tr("Login screen");
}

QString LoginSurface::defaultReadPath()
{
    // Native install: /etc/plasmalogin.conf is the host's file.
    // Flatpak: /etc/ inside the sandbox is the runtime's /etc, NOT
    // the host's. With --filesystem=host:ro the host's /etc/ is
    // bind-mounted at /run/host/etc/. Detect Flatpak via the
    // sentinel /.flatpak-info and route the read there.
    if (QFileInfo::exists(kFlatpakInfoMarker)) {
        return kFlatpakHostReadPath;
    }
    return kHostConfigPath;
}

QString LoginSurface::defaultWritePath()
{
    // The privileged helper runs on the host (or, under Flatpak,
    // would-run via system D-Bus + polkit) and validates the path it
    // writes to against the canonical /etc/plasmalogin.conf. So the
    // write path is always the host-canonical path, regardless of
    // where the client reads from.
    return kHostConfigPath;
}

QString LoginSurface::currentImagePath() const
{
    KConfig config(m_readPath, KConfig::SimpleConfig);
    const KConfigGroup imageGroup = config.group(kGreeter)
                                        .group(kWallpaper)
                                        .group(kImagePlugin)
                                        .group(kGeneral);
    return imageGroup.readEntry(kImageKey, QString());
}

void LoginSurface::apply(const QString &imagePath)
{
    if (!m_writer) {
        Q_EMIT applyFailed(tr("No privileged writer configured"));
        return;
    }

    QString normalized = imagePath;
    if (!normalized.startsWith(QLatin1String("file://"))
        && !normalized.startsWith(QLatin1String("http"))) {
        normalized = QUrl::fromLocalFile(imagePath).toString();
    }

    // Stage a user-writable copy so we can mutate it with KConfig
    // without touching the privileged file. If the source doesn't
    // exist (fresh system), we start from an empty staging file.
    QTemporaryFile staging;
    staging.setAutoRemove(true);
    if (!staging.open()) {
        Q_EMIT applyFailed(tr("Cannot create staging file"));
        return;
    }
    const QString stagingPath = staging.fileName();
    staging.close();

    if (QFile::exists(m_readPath)) {
        QFile::remove(stagingPath);
        if (!QFile::copy(m_readPath, stagingPath)) {
            Q_EMIT applyFailed(tr("Cannot stage %1").arg(m_readPath));
            return;
        }
    }

    {
        KConfig config(stagingPath, KConfig::SimpleConfig);
        KConfigGroup imageGroup = config.group(kGreeter)
                                      .group(kWallpaper)
                                      .group(kImagePlugin)
                                      .group(kGeneral);
        imageGroup.writeEntry(kImageKey, normalized);
        if (!config.sync()) {
            Q_EMIT applyFailed(tr("Cannot stage edit to %1").arg(stagingPath));
            return;
        }
    }

    QFile staged(stagingPath);
    if (!staged.open(QIODevice::ReadOnly)) {
        Q_EMIT applyFailed(tr("Cannot read staged file"));
        return;
    }
    const QByteArray contents = staged.readAll();
    staged.close();

    m_writer->writeAtomically(m_writePath, contents);
}

void LoginSurface::wireWriterSignals()
{
    if (!m_writer) {
        return;
    }
    connect(m_writer, &PrivilegedWriter::writeSucceeded,
            this, [this](const QString &path) {
                if (path != m_writePath) {
                    return;
                }
                Q_EMIT currentImagePathChanged();
                Q_EMIT applySucceeded();
            });
    connect(m_writer, &PrivilegedWriter::writeFailed,
            this, [this](const QString &path, const QString &reason) {
                if (path != m_writePath) {
                    return;
                }
                Q_EMIT applyFailed(reason);
            });
}
