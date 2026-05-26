// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LockscreenSurface.h"

#include <QDir>
#include <QFile>
#include <QObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

class TestLockscreenSurface : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void identityAndDisplayName();
    void readsCurrentImageFromFixture();
    void emptyImageWhenConfigMissing();
    void applyWritesImageBackAsFileUrl();
    void applyKeepsExistingFileUrlPrefix();
    void applyCreatesConfigIfAbsent();

private:
    QString copyFixtureToTemp(const QString &fixtureName);
    static QString fixturePath(const QString &fixtureName);
};

void TestLockscreenSurface::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void TestLockscreenSurface::identityAndDisplayName()
{
    LockscreenSurface s(QStringLiteral("/dev/null"));
    QCOMPARE(s.id(), QStringLiteral("lockscreen"));
    QVERIFY(!s.displayName().isEmpty());
}

void TestLockscreenSurface::readsCurrentImageFromFixture()
{
    LockscreenSurface s(fixturePath(QStringLiteral("sample-kscreenlockerrc.ini")));
    QCOMPARE(s.currentImagePath(),
             QStringLiteral("file:///usr/share/wallpapers/Patak/contents/images/2880x1800.png"));
}

void TestLockscreenSurface::emptyImageWhenConfigMissing()
{
    LockscreenSurface s(QStringLiteral("/nonexistent/path/that/does/not/exist"));
    QCOMPARE(s.currentImagePath(), QString());
}

void TestLockscreenSurface::applyWritesImageBackAsFileUrl()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-kscreenlockerrc.ini"));
    QVERIFY(!temp.isEmpty());

    LockscreenSurface s(temp);
    QSignalSpy success(&s, &WallpaperSurface::applySucceeded);
    QSignalSpy failed(&s, &WallpaperSurface::applyFailed);
    QSignalSpy changed(&s, &WallpaperSurface::currentImagePathChanged);

    s.apply(QStringLiteral("/home/test/Pictures/lockscreen.jpg"));

    QCOMPARE(success.count(), 1);
    QCOMPARE(failed.count(), 0);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(s.currentImagePath(),
             QStringLiteral("file:///home/test/Pictures/lockscreen.jpg"));

    QFile::remove(temp);
}

void TestLockscreenSurface::applyKeepsExistingFileUrlPrefix()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-kscreenlockerrc.ini"));
    QVERIFY(!temp.isEmpty());

    LockscreenSurface s(temp);
    s.apply(QStringLiteral("file:///already/prefixed.png"));

    QCOMPARE(s.currentImagePath(), QStringLiteral("file:///already/prefixed.png"));
    QFile::remove(temp);
}

void TestLockscreenSurface::applyCreatesConfigIfAbsent()
{
    // Lock screen group path is fixed — unlike desktop containments
    // we can write into a brand-new file. This is the behavior that
    // makes "first-time apply" work on systems where the user never
    // touched the lock screen settings.
    const QString temp = QDir::temp().filePath(QStringLiteral("pws-test-fresh-kscreenlockerrc.ini"));
    QFile::remove(temp);

    LockscreenSurface s(temp);
    QSignalSpy success(&s, &WallpaperSurface::applySucceeded);

    s.apply(QStringLiteral("/home/test/new-lock.png"));

    QCOMPARE(success.count(), 1);
    QCOMPARE(s.currentImagePath(), QStringLiteral("file:///home/test/new-lock.png"));

    QFile::remove(temp);
}

QString TestLockscreenSurface::copyFixtureToTemp(const QString &fixtureName)
{
    const QString src = fixturePath(fixtureName);
    const QString dst = QDir::temp().filePath(QStringLiteral("pws-test-") + fixtureName);
    QFile::remove(dst);
    if (!QFile::copy(src, dst)) {
        return {};
    }
    return dst;
}

QString TestLockscreenSurface::fixturePath(const QString &fixtureName)
{
    return QStringLiteral(PWS_TEST_FIXTURES_DIR "/") + fixtureName;
}

QTEST_GUILESS_MAIN(TestLockscreenSurface)
#include "tst_LockscreenSurface.moc"
