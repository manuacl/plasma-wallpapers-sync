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
    void skipsPanelContainmentEvenWithWallpaperplugin();
    void applyWritesToEveryDesktopContainment();
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

void TestDesktopSurface::skipsPanelContainmentEvenWithWallpaperplugin()
{
    // Regression for the "Applied to lockscreen, failed on desktop"
    // bug: the panel containment carries wallpaperplugin=org.kde.image
    // on a real Plasma 6 install too. Detection by Image-entry
    // existence must skip it and land on the real desktop containment.
    // Our fixture already has this shape (panel = [1], desktop = [2]);
    // verify the returned image is the desktop's, not blank.
    DesktopSurface s(fixturePath(QStringLiteral("sample-desktop-appletsrc.ini")));
    QCOMPARE(s.currentImagePath(),
             QStringLiteral("file:///home/test/Pictures/wallpaper1.jpg"));
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

void TestDesktopSurface::applyWritesToEveryDesktopContainment()
{
    // Multi-screen Plasma setups carry one containment per monitor
    // under the same activity. Apply must write to every containment
    // that has a wallpaper subgroup — otherwise the "sync" promise
    // only covers one screen and the user sees the new wallpaper on
    // one monitor but the old one on the other.
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-desktop-appletsrc-multi.ini"));
    QVERIFY(!temp.isEmpty());

    DesktopSurface s(temp);
    QSignalSpy success(&s, &WallpaperSurface::applySucceeded);
    s.apply(QStringLiteral("/home/test/Pictures/synced.png"));
    QCOMPARE(success.count(), 1);

    // Re-read both containments through a fresh KConfig and assert
    // both got the new image (currentImagePath only returns the
    // primary — that's fine, but the apply must have hit both).
    QFile f(temp);
    QVERIFY(f.open(QIODevice::ReadOnly));
    const QByteArray after = f.readAll();
    f.close();
    QVERIFY(after.contains("[Containments][2][Wallpaper][org.kde.image][General]"));
    QVERIFY(after.contains("[Containments][27][Wallpaper][org.kde.image][General]"));
    const int matches = after.count("file:///home/test/Pictures/synced.png");
    QCOMPARE(matches, 2);

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
