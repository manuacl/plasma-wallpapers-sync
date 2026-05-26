// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PWS_TESTS_FAKE_PRIVILEGED_WRITER_H
#define PWS_TESTS_FAKE_PRIVILEGED_WRITER_H

#include "PrivilegedWriter.h"

#include <QFile>

/**
 * Test double for PrivilegedWriter.
 *
 * Performs the write directly as the test user — useful because the
 * tests can't elevate, and because we want to verify that the bytes
 * LoginSurface produces are well-formed by re-reading them through a
 * second LoginSurface afterwards.
 *
 * setFailureReason() lets a test exercise the failure path without
 * touching the filesystem at all.
 */
class FakePrivilegedWriter : public PrivilegedWriter
{
    Q_OBJECT
public:
    explicit FakePrivilegedWriter(QObject *parent = nullptr)
        : PrivilegedWriter(parent)
    {
    }

    void setFailureReason(const QString &reason) { m_failureReason = reason; }
    void clearFailure() { m_failureReason.clear(); }

    int callCount() const { return m_callCount; }
    QString lastPath() const { return m_lastPath; }
    QByteArray lastContents() const { return m_lastContents; }

public Q_SLOTS:
    void writeAtomically(const QString &path, const QByteArray &contents) override
    {
        ++m_callCount;
        m_lastPath = path;
        m_lastContents = contents;

        if (!m_failureReason.isEmpty()) {
            Q_EMIT writeFailed(path, m_failureReason);
            return;
        }
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            Q_EMIT writeFailed(path, f.errorString());
            return;
        }
        f.write(contents);
        f.close();
        Q_EMIT writeSucceeded(path);
    }

private:
    QString m_failureReason;
    int m_callCount = 0;
    QString m_lastPath;
    QByteArray m_lastContents;
};

#endif // PWS_TESTS_FAKE_PRIVILEGED_WRITER_H
