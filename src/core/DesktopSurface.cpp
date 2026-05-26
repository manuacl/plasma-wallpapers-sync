// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DesktopSurface.h"

#include <KConfig>
#include <KConfigGroup>
#include <QDir>
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

QString DesktopSurface::description() const
{
    return tr("Shown on the active Plasma desktop, behind windows and the panel.");
}

QString DesktopSurface::defaultConfigPath()
{
    // Hard-target $HOME/.config rather than $XDG_CONFIG_HOME via
    // QStandardPaths::ConfigLocation: the file we want is the
    // user-wide Plasma config that the live session reads, which by
    // convention lives in ~/.config/ regardless of XDG_CONFIG_HOME's
    // value. Inside the Flatpak sandbox the runtime forces
    // XDG_CONFIG_HOME to the per-app private dir even with
    // --unset-env in finish-args, so the only reliable read/write
    // path to the host's plasma config is $HOME/.config (bind-mounted
    // via --filesystem=xdg-config).
    return QDir::homePath() + QStringLiteral("/.config/plasma-org.kde.plasma.desktop-appletsrc");
}

QString DesktopSurface::findPrimaryContainmentId() const
{
    const QStringList ids = findAllDesktopContainmentIds();
    return ids.isEmpty() ? QString() : ids.first();
}

QStringList DesktopSurface::findAllDesktopContainmentIds() const
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
    QStringList result;
    for (const QString &id : ids) {
        const KConfigGroup imageGroup = containments.group(id)
                                            .group(kWallpaper)
                                            .group(kImagePlugin)
                                            .group(kGeneral);
        if (imageGroup.hasKey(kImageKey)) {
            result.append(id);
        }
    }
    return result;
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
    const QStringList containmentIds = findAllDesktopContainmentIds();
    if (containmentIds.isEmpty()) {
        Q_EMIT applyFailed(tr("No desktop containment found in %1").arg(m_configPath));
        return;
    }

    // Plasma stores the image as a file:// URL — normalize bare paths.
    QString normalized = imagePath;
    if (!normalized.startsWith(QLatin1String("file://"))
        && !normalized.startsWith(QLatin1String("http"))) {
        normalized = QUrl::fromLocalFile(imagePath).toString();
    }

    // Sync the wallpaper across every desktop containment (= every
    // screen). Per-screen differentiation is explicitly out of scope
    // for v1 — this is the "sync" in the project's name.
    KConfig config(m_configPath, KConfig::SimpleConfig);
    for (const QString &containmentId : containmentIds) {
        KConfigGroup imageGroup = config.group(kContainments)
                                      .group(containmentId)
                                      .group(kWallpaper)
                                      .group(kImagePlugin)
                                      .group(kGeneral);
        imageGroup.writeEntry(kImageKey, normalized);
    }
    if (!config.sync()) {
        Q_EMIT applyFailed(tr("Failed to write %1").arg(m_configPath));
        return;
    }
    Q_EMIT currentImagePathChanged();
    Q_EMIT applySucceeded();
}
