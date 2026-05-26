// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DesktopSurface.h"

#include <KConfig>
#include <KConfigGroup>
#include <QStandardPaths>
#include <QUrl>

namespace
{
const QString kContainments = QStringLiteral("Containments");
const QString kWallpaper = QStringLiteral("Wallpaper");
const QString kImagePlugin = QStringLiteral("org.kde.image");
const QString kGeneral = QStringLiteral("General");
const QString kImageKey = QStringLiteral("Image");
}

DesktopSurface::DesktopSurface(QObject *parent)
    : DesktopSurface(defaultConfigPath(), parent)
{
}

DesktopSurface::DesktopSurface(const QString &configPath, QObject *parent)
    : WallpaperSurface(parent)
    , m_configPath(configPath)
{
}

DesktopSurface::~DesktopSurface() = default;

QString DesktopSurface::id() const
{
    return QStringLiteral("desktop");
}

QString DesktopSurface::displayName() const
{
    return tr("Desktop");
}

QString DesktopSurface::defaultConfigPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
        + QStringLiteral("/plasma-org.kde.plasma.desktop-appletsrc");
}

QString DesktopSurface::findPrimaryContainmentId() const
{
    // Detect by structure rather than by the containment's `plugin` value:
    // Plasma 5 used `org.kde.desktopcontainment`, Plasma 6 uses
    // `org.kde.plasma.folder`, and the panel containment carries a
    // `wallpaperplugin=org.kde.image` entry too even though it has no
    // wallpaper. The reliable signal is the presence of a non-empty
    // `[Wallpaper][org.kde.image][General]` subgroup with an `Image`
    // entry — that only ever appears on actual desktop surfaces.
    KConfig config(m_configPath, KConfig::SimpleConfig);
    const KConfigGroup containments = config.group(kContainments);
    const QStringList ids = containments.groupList();
    for (const QString &id : ids) {
        const KConfigGroup imageGroup = containments.group(id)
                                            .group(kWallpaper)
                                            .group(kImagePlugin)
                                            .group(kGeneral);
        if (imageGroup.hasKey(kImageKey)) {
            return id;
        }
    }
    return {};
}

QString DesktopSurface::currentImagePath() const
{
    const QString containmentId = findPrimaryContainmentId();
    if (containmentId.isEmpty()) {
        return {};
    }
    KConfig config(m_configPath, KConfig::SimpleConfig);
    const KConfigGroup imageGroup = config.group(kContainments)
                                        .group(containmentId)
                                        .group(kWallpaper)
                                        .group(kImagePlugin)
                                        .group(kGeneral);
    return imageGroup.readEntry(kImageKey, QString());
}

void DesktopSurface::apply(const QString &imagePath)
{
    const QString containmentId = findPrimaryContainmentId();
    if (containmentId.isEmpty()) {
        Q_EMIT applyFailed(tr("No desktop containment found in %1").arg(m_configPath));
        return;
    }

    // Plasma stores the image as a file:// URL — normalize bare paths.
    QString normalized = imagePath;
    if (!normalized.startsWith(QLatin1String("file://"))
        && !normalized.startsWith(QLatin1String("http"))) {
        normalized = QUrl::fromLocalFile(imagePath).toString();
    }

    KConfig config(m_configPath, KConfig::SimpleConfig);
    KConfigGroup imageGroup = config.group(kContainments)
                                  .group(containmentId)
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
