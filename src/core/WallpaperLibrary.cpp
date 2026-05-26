// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "WallpaperLibrary.h"

#include <QDir>
#include <QFileInfo>
#include <QSet>
#include <QUrl>

namespace
{
const QStringList kImageGlobs = {
    QStringLiteral("*.jpg"),
    QStringLiteral("*.jpeg"),
    QStringLiteral("*.png"),
    QStringLiteral("*.webp"),
    QStringLiteral("*.bmp"),
    QStringLiteral("*.jxl"),
};
}

WallpaperLibrary::WallpaperLibrary(QObject *parent)
    : QAbstractListModel(parent)
    , m_searchPaths(defaultSearchPaths())
{
    scan();
}

WallpaperLibrary::~WallpaperLibrary() = default;

int WallpaperLibrary::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_entries.size();
}

QVariant WallpaperLibrary::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }
    const WallpaperEntry &e = m_entries.at(index.row());
    switch (role) {
    case NameRole:
        return e.name;
    case PreviewPathRole:
        return QUrl::fromLocalFile(e.previewPath).toString();
    case ApplyPathRole:
        return QUrl::fromLocalFile(e.applyPath).toString();
    default:
        return {};
    }
}

QHash<int, QByteArray> WallpaperLibrary::roleNames() const
{
    return {
        {NameRole, "name"},
        {PreviewPathRole, "previewPath"},
        {ApplyPathRole, "applyPath"},
    };
}

QStringList WallpaperLibrary::searchPaths() const
{
    return m_searchPaths;
}

void WallpaperLibrary::setSearchPaths(const QStringList &paths)
{
    m_searchPaths = paths;
    scan();
}

void WallpaperLibrary::reload()
{
    scan();
}

QStringList WallpaperLibrary::defaultSearchPaths()
{
    return {
        // Native install: where Plasma ships its bundled wallpapers.
        QStringLiteral("/usr/share/wallpapers"),
        // Flatpak install: /usr is reserved by Flatpak, host's
        // /usr/share/wallpapers is reachable through /run/host/ under
        // --filesystem=host:ro. Non-existent on a native install,
        // skipped silently by scanDir().
        QStringLiteral("/run/host/usr/share/wallpapers"),
        // User-installed packages and quick-drop folders.
        QDir::homePath() + QStringLiteral("/.local/share/wallpapers"),
    };
}

void WallpaperLibrary::scan()
{
    beginResetModel();
    m_entries.clear();
    for (const QString &path : std::as_const(m_searchPaths)) {
        scanDir(path);
    }
    endResetModel();
}

void WallpaperLibrary::scanDir(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return;
    }

    // Names already added across previous search paths win — keeps
    // user overrides at ~/.local/share/wallpapers/ from being shadowed
    // by a system package with the same name (though strictly the
    // ordering depends on m_searchPaths).
    QSet<QString> seenNames;
    for (const WallpaperEntry &e : std::as_const(m_entries)) {
        seenNames.insert(e.name);
    }

    const QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    QSet<QString> packageSubdirs;

    // Package wallpapers: <name>/contents/images/*.<ext>
    for (const QString &subdir : subdirs) {
        const QString imagesDir = dirPath + QLatin1Char('/') + subdir
            + QStringLiteral("/contents/images");
        const QString image = firstImageInDir(imagesDir);
        if (image.isEmpty()) {
            continue;
        }
        packageSubdirs.insert(subdir);
        if (seenNames.contains(subdir)) {
            continue;
        }
        m_entries.append({subdir, image, image});
        seenNames.insert(subdir);
    }

    // Flat images: dropped directly under the search dir.
    const QStringList flatFiles = dir.entryList(kImageGlobs, QDir::Files, QDir::Name);
    for (const QString &file : flatFiles) {
        const QString name = QFileInfo(file).completeBaseName();
        if (seenNames.contains(name)) {
            continue;
        }
        const QString path = dirPath + QLatin1Char('/') + file;
        m_entries.append({name, path, path});
        seenNames.insert(name);
    }

    // Flat images inside a non-package subdir, one level deep —
    // the common "I dumped a few images under ~/.local/share/wallpapers/<topic>/"
    // user shape. Each image is its own entry, so the user picks the
    // image, not the folder.
    for (const QString &subdir : subdirs) {
        if (packageSubdirs.contains(subdir)) {
            continue;
        }
        QDir sub(dirPath + QLatin1Char('/') + subdir);
        const QStringList subFiles = sub.entryList(kImageGlobs, QDir::Files, QDir::Name);
        for (const QString &file : subFiles) {
            const QString name = QFileInfo(file).completeBaseName();
            if (seenNames.contains(name)) {
                continue;
            }
            const QString path = sub.absoluteFilePath(file);
            m_entries.append({name, path, path});
            seenNames.insert(name);
        }
    }
}

QString WallpaperLibrary::firstImageInDir(const QString &dirPath)
{
    QDir dir(dirPath);
    if (!dir.exists()) {
        return {};
    }
    const QStringList files = dir.entryList(kImageGlobs, QDir::Files, QDir::Name);
    if (files.isEmpty()) {
        return {};
    }
    return dirPath + QLatin1Char('/') + files.first();
}
