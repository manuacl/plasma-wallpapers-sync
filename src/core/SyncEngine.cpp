// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "SyncEngine.h"

#include "WallpaperSurface.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(lcSyncEngine, "pws.core.syncengine")

SyncEngine::SyncEngine(QObject *parent)
    : QObject(parent)
{
}

SyncEngine::~SyncEngine() = default;

void SyncEngine::addSurface(WallpaperSurface *s)
{
    if (!s) {
        return;
    }
    const QString id = s->id();
    if (m_surfaces.contains(id)) {
        return;
    }
    m_surfaces.insert(id, s);
    m_order.append(id);
    Q_EMIT surfacesChanged();

    connect(s, &WallpaperSurface::applySucceeded, this, [this, id]() {
        Q_EMIT surfaceApplySucceeded(id);
        if (m_pending.remove(id)) {
            m_succeeded.append(id);
            finishIfDone();
        }
    });
    connect(s, &WallpaperSurface::applyFailed, this, [this, id](const QString &reason) {
        Q_EMIT surfaceApplyFailed(id, reason);
        if (m_pending.remove(id)) {
            m_failed.append(id);
            finishIfDone();
        }
    });
}

QStringList SyncEngine::surfaceIds() const
{
    return m_order;
}

WallpaperSurface *SyncEngine::surface(const QString &id) const
{
    return m_surfaces.value(id, nullptr);
}

bool SyncEngine::isApplying() const
{
    return !m_pending.isEmpty();
}

void SyncEngine::applyToSurfaces(const QString &imagePath, const QStringList &targetIds)
{
    if (isApplying()) {
        qCWarning(lcSyncEngine)
            << "ignoring applyToSurfaces while a batch is in flight";
        return;
    }

    // Resolve targets and seed the pending set BEFORE dispatching any
    // surface.apply() — sync surfaces (Desktop/Lockscreen) emit their
    // result inside the call, so all targets must already be tracked.
    QList<WallpaperSurface *> targets;
    for (const QString &id : targetIds) {
        if (auto *s = m_surfaces.value(id)) {
            targets.append(s);
            m_pending.insert(id);
        }
    }

    if (m_pending.isEmpty()) {
        Q_EMIT applyFinished({}, {});
        return;
    }

    for (auto *s : targets) {
        s->apply(imagePath);
    }
}

void SyncEngine::applyToAll(const QString &imagePath)
{
    applyToSurfaces(imagePath, m_order);
}

void SyncEngine::finishIfDone()
{
    if (!m_pending.isEmpty()) {
        return;
    }
    const QStringList succeeded = std::move(m_succeeded);
    const QStringList failed = std::move(m_failed);
    m_succeeded.clear();
    m_failed.clear();
    Q_EMIT applyFinished(succeeded, failed);
}
