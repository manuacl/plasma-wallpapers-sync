// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "DesktopSurface.h"

#include <QDir>
#include <QFile>
#include <QObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTest>

class TestDesktopSurface : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void identityAndDisplayName();
    void readsCurrentImageFromFixture();
    void emptyImageWhenNoMatchingContainment();
    void applyWritesImageBackAsFileUrl();
    void applyKeepsExistingFileUrlPrefix();
    void applyFailsWhenNoContainment();

private:
    QString copyFixtureToTemp(const QString &fixtureName);
    static QString fixturePath(const QString &fixtureName);
};

void TestDesktopSurface::initTestCase()
{
    // Keep KConfig from touching the real user config under $HOME during tests.
    QStandardPaths::setTestModeEnabled(true);
}

void TestDesktopSurface::identityAndDisplayName()
{
    DesktopSurface s(QStringLiteral("/dev/null"));
    QCOMPARE(s.id(), QStringLiteral("desktop"));
    QVERIFY(!s.displayName().isEmpty());
}

void TestDesktopSurface::readsCurrentImageFromFixture()
{
    DesktopSurface s(fixturePath(QStringLiteral("sample-desktop-appletsrc.ini")));
    QCOMPARE(s.currentImagePath(),
             QStringLiteral("file:///home/test/Pictures/wallpaper1.jpg"));
}

void TestDesktopSurface::emptyImageWhenNoMatchingContainment()
{
    QTemporaryFile empty;
    QVERIFY(empty.open());
    empty.close();
    DesktopSurface s(empty.fileName());
    QCOMPARE(s.currentImagePath(), QString());
}

void TestDesktopSurface::applyWritesImageBackAsFileUrl()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-desktop-appletsrc.ini"));
    QVERIFY(!temp.isEmpty());

    DesktopSurface s(temp);
    QSignalSpy success(&s, &WallpaperSurface::applySucceeded);
    QSignalSpy failed(&s, &WallpaperSurface::applyFailed);
    QSignalSpy changed(&s, &WallpaperSurface::currentImagePathChanged);

    s.apply(QStringLiteral("/home/test/Pictures/new.jpg"));

    QCOMPARE(success.count(), 1);
    QCOMPARE(failed.count(), 0);
    QCOMPARE(changed.count(), 1);
    QCOMPARE(s.currentImagePath(),
             QStringLiteral("file:///home/test/Pictures/new.jpg"));

    QFile::remove(temp);
}

void TestDesktopSurface::applyKeepsExistingFileUrlPrefix()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-desktop-appletsrc.ini"));
    QVERIFY(!temp.isEmpty());

    DesktopSurface s(temp);
    s.apply(QStringLiteral("file:///already/prefixed.png"));

    QCOMPARE(s.currentImagePath(), QStringLiteral("file:///already/prefixed.png"));
    QFile::remove(temp);
}

void TestDesktopSurface::applyFailsWhenNoContainment()
{
    QTemporaryFile empty;
    QVERIFY(empty.open());
    empty.close();

    DesktopSurface s(empty.fileName());
    QSignalSpy success(&s, &WallpaperSurface::applySucceeded);
    QSignalSpy failed(&s, &WallpaperSurface::applyFailed);

    s.apply(QStringLiteral("/tmp/whatever.jpg"));

    QCOMPARE(success.count(), 0);
    QCOMPARE(failed.count(), 1);
}

QString TestDesktopSurface::copyFixtureToTemp(const QString &fixtureName)
{
    const QString src = fixturePath(fixtureName);
    const QString dst = QDir::temp().filePath(QStringLiteral("pws-test-") + fixtureName);
    QFile::remove(dst);
    if (!QFile::copy(src, dst)) {
        return {};
    }
    return dst;
}

QString TestDesktopSurface::fixturePath(const QString &fixtureName)
{
    return QStringLiteral(PWS_TEST_FIXTURES_DIR "/") + fixtureName;
}

QTEST_GUILESS_MAIN(TestDesktopSurface)
#include "tst_DesktopSurface.moc"
