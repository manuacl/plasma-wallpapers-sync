// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PWS_CORE_WALLPAPER_LIBRARY_H
#define PWS_CORE_WALLPAPER_LIBRARY_H

#include <QAbstractListModel>
#include <QString>
#include <QStringList>

struct WallpaperEntry
{
    QString name;
    QString previewPath; // bare filesystem path (no file:// prefix)
    QString applyPath;
};

/**
 * Read-only model enumerating wallpapers found on disk under a
 * configurable set of search paths.
 *
 * Two layout conventions are supported in a single pass:
 *
 *   1. *Package wallpapers* — a `contents/images/` directory under
 *      any `<root>/<name>/` subdir, containing one or more image
 *      files. The convention `/usr/share/wallpapers/` and most
 *      KDE-curated themes follow. Picks the alphabetically-first
 *      image inside `contents/images/` for both the thumbnail and
 *      the apply path; Plasma's image renderer happily consumes any
 *      single resolution from a package.
 *
 *   2. *Flat image files* — bare image files dropped under `<root>/`
 *      directly OR one level deep in a non-package subdir (e.g.
 *      `~/.local/share/wallpapers/Personal/sunset.jpg`). Each image
 *      becomes its own entry; the subdir name itself isn't shown.
 *
 * Default search paths: `/usr/share/wallpapers/` (the runtime's
 * bundle) and `$HOME/.local/share/wallpapers/`. Tests inject their
 * own via setSearchPaths().
 *
 * Stays in core/ — QtCore-only, no GUI, no Kirigami, no DBus, no
 * KAuth. The plasma-isolation seam keeps it portable to a future
 * KCM port.
 */
class WallpaperLibrary : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        PreviewPathRole,
        ApplyPathRole,
    };

    explicit WallpaperLibrary(QObject *parent = nullptr);
    ~WallpaperLibrary() override;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QStringList searchPaths() const;
    void setSearchPaths(const QStringList &paths);

public Q_SLOTS:
    void reload();

private:
    static QStringList defaultSearchPaths();
    void scan();
    void scanDir(const QString &dirPath);
    static QString firstImageInDir(const QString &dirPath);

    QStringList m_searchPaths;
    QList<WallpaperEntry> m_entries;
};

#endif // PWS_CORE_WALLPAPER_LIBRARY_H
