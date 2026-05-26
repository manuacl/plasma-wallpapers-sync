// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PWS_TESTS_FAKE_SURFACE_H
#define PWS_TESTS_FAKE_SURFACE_H

#include "WallpaperSurface.h"

/**
 * In-memory WallpaperSurface used by tst_SyncEngine. The real
 * surfaces (Desktop/Lockscreen/Login) drag in KConfig fixtures and
 * temp files; this one keeps the engine tests focused on the
 * coordination logic by exercising signals directly.
 */
class FakeSurface : public WallpaperSurface
{
    Q_OBJECT
public:
    explicit FakeSurface(const QString &id, QObject *parent = nullptr)
        : WallpaperSurface(parent)
        , m_id(id)
        , m_displayName(id)
    {
    }

    QString id() const override { return m_id; }
    QString displayName() const override { return m_displayName; }
    QString currentImagePath() const override { return m_currentImagePath; }

    void setFailureReason(const QString &reason) { m_failureReason = reason; }

    int applyCallCount() const { return m_applyCount; }
    QString lastApplyPath() const { return m_lastApplyPath; }

public Q_SLOTS:
    void apply(const QString &imagePath) override
    {
        ++m_applyCount;
        m_lastApplyPath = imagePath;
        if (!m_failureReason.isEmpty()) {
            Q_EMIT applyFailed(m_failureReason);
            return;
        }
        m_currentImagePath = imagePath;
        Q_EMIT currentImagePathChanged();
        Q_EMIT applySucceeded();
    }

private:
    QString m_id;
    QString m_displayName;
    QString m_currentImagePath;
    QString m_failureReason;
    int m_applyCount = 0;
    QString m_lastApplyPath;
};

#endif // PWS_TESTS_FAKE_SURFACE_H
