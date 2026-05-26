// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LockscreenSurface.h"

#include <KConfig>
#include <KConfigGroup>
#include <QStandardPaths>
#include <QUrl>

namespace
{
const QString kGreeter = QStringLiteral("Greeter");
const QString kWallpaper = QStringLiteral("Wallpaper");
const QString kImagePlugin = QStringLiteral("org.kde.image");
const QString kGeneral = QStringLiteral("General");
const QString kImageKey = QStringLiteral("Image");
}

LockscreenSurface::LockscreenSurface(QObject *parent)
    : LockscreenSurface(defaultConfigPath(), parent)
{
}

LockscreenSurface::LockscreenSurface(const QString &configPath, QObject *parent)
    : WallpaperSurface(parent)
    , m_configPath(configPath)
{
}

LockscreenSurface::~LockscreenSurface() = default;

QString LockscreenSurface::id() const
{
    return QStringLiteral("lockscreen");
}

QString LockscreenSurface::displayName() const
{
    return tr("Lock screen");
}

QString LockscreenSurface::defaultConfigPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/kscreenlockerrc");
}

QString LockscreenSurface::currentImagePath() const
{
    KConfig config(m_configPath, KConfig::SimpleConfig);
    const KConfigGroup imageGroup = config.group(kGreeter)
                                        .group(kWallpaper)
                                        .group(kImagePlugin)
                                        .group(kGeneral);
    return imageGroup.readEntry(kImageKey, QString());
}

void LockscreenSurface::apply(const QString &imagePath)
{
    QString normalized = imagePath;
    if (!normalized.startsWith(QLatin1String("file://"))
        && !normalized.startsWith(QLatin1String("http"))) {
        normalized = QUrl::fromLocalFile(imagePath).toString();
    }

    KConfig config(m_configPath, KConfig::SimpleConfig);
    KConfigGroup imageGroup = config.group(kGreeter)
                                  .group(kWallpaper)
                                  .group(kImagePlugin)
                                  .group(kGeneral);
    imageGroup.writeEntry(kImageKey, normalized);
    if (!config.sync()) {
        Q_EMIT applyFailed(tr("Failed to write %1").arg(m_configPath));
        return;
    }
    Q_EMIT currentImagePathChanged();
    Q_EMIT applySucceeded();
}
