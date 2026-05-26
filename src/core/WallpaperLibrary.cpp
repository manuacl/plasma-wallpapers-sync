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
        QStringLiteral("/usr/share/wallpapers"),
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

    // Package wallpapers: <name>/contents/images/*.<ext>
    const QStringList subdirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QString &subdir : subdirs) {
        const QString imagesDir = dirPath + QLatin1Char('/') + subdir
            + QStringLiteral("/contents/images");
        const QString image = firstImageInDir(imagesDir);
        if (!image.isEmpty() && !seenNames.contains(subdir)) {
            m_entries.append({subdir, image, image});
            seenNames.insert(subdir);
        }
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
