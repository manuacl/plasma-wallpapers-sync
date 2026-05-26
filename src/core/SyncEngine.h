// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PWS_CORE_SYNC_ENGINE_H
#define PWS_CORE_SYNC_ENGINE_H

#include <QHash>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>

class WallpaperSurface;

/**
 * Coordinator that fans out a single "apply this image" request
 * across a chosen subset of registered WallpaperSurfaces and
 * aggregates their async outcomes into one applyFinished signal.
 *
 * Ownership: the engine does NOT take ownership of surfaces. The
 * shell instantiates them (so it can wire up the PrivilegedWriter
 * for LoginSurface), then hands non-owning pointers to addSurface().
 *
 * Concurrency: only one batch may be in flight at a time. Calling
 * applyToSurfaces() / applyToAll() while a batch is pending is a
 * no-op (logged via qWarning) — the GUI should disable the Apply
 * button while waiting on applyFinished.
 *
 * Empty target list: emits applyFinished({}, {}) synchronously so
 * the caller never has to special-case "nothing selected".
 *
 * Unknown surface ids: silently dropped from the batch. They don't
 * appear in either succeeded or failed.
 */
class SyncEngine : public QObject
{
    Q_OBJECT
public:
    explicit SyncEngine(QObject *parent = nullptr);
    ~SyncEngine() override;

    void addSurface(WallpaperSurface *surface);
    QStringList surfaceIds() const;
    WallpaperSurface *surface(const QString &id) const;
    bool isApplying() const;

public Q_SLOTS:
    void applyToSurfaces(const QString &imagePath, const QStringList &targetIds);
    void applyToAll(const QString &imagePath);

Q_SIGNALS:
    void surfaceApplySucceeded(const QString &id);
    void surfaceApplyFailed(const QString &id, const QString &reason);
    void applyFinished(const QStringList &succeeded, const QStringList &failed);

private:
    void finishIfDone();

    QHash<QString, WallpaperSurface *> m_surfaces;
    QStringList m_order;

    QSet<QString> m_pending;
    QStringList m_succeeded;
    QStringList m_failed;
};

#endif // PWS_CORE_SYNC_ENGINE_H
