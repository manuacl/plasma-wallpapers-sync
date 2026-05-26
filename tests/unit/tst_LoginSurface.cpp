// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FakePrivilegedWriter.h"
#include "LoginSurface.h"

#include <QDir>
#include <QFile>
#include <QObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

class TestLoginSurface : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void identityAndDisplayName();
    void readsCurrentImageFromFixture();
    void emptyImageWhenConfigMissing();
    void applyInstallsToPlasmaloginWallpapersDir();
    void applyConfPointsAtInstalledCopy();
    void applyEmitsSucceededAndUpdatesFile();
    void applyKeepsBasenameFromFileUrl();
    void applyEmitsFailedWhenInstallFails();
    void applyDoesNotWriteConfWhenInstallFails();
    void applyEmitsFailedWhenWriteFails();
    void applyWorksWhenSourceFileMissing();
    void readPathAndWritePathCanDiffer();
    void previewFallsBackToCurrentWhenNoCache();
    void previewUsesCachedSourceAfterApply();
    void previewFallsBackWhenCachedDestNoLongerMatches();
    void previewSourceCacheNotWrittenWhenInstallFails();

private:
    QString copyFixtureToTemp(const QString &fixtureName);
    static QString fixturePath(const QString &fixtureName);
};

void TestLoginSurface::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void TestLoginSurface::identityAndDisplayName()
{
    FakePrivilegedWriter writer;
    LoginSurface s(&writer, QStringLiteral("/dev/null"));
    QCOMPARE(s.id(), QStringLiteral("login"));
    QVERIFY(!s.displayName().isEmpty());
}

void TestLoginSurface::readsCurrentImageFromFixture()
{
    FakePrivilegedWriter writer;
    LoginSurface s(&writer, fixturePath(QStringLiteral("sample-plasmalogin.conf")));
    QCOMPARE(s.currentImagePath(),
             QStringLiteral("file:///var/lib/plasmalogin/wallpapers/initial-d-ae86.jpg"));
}

void TestLoginSurface::emptyImageWhenConfigMissing()
{
    FakePrivilegedWriter writer;
    LoginSurface s(&writer, QStringLiteral("/nonexistent/path/that/does/not/exist"));
    QCOMPARE(s.currentImagePath(), QString());
}

void TestLoginSurface::applyInstallsToPlasmaloginWallpapersDir()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!temp.isEmpty());

    FakePrivilegedWriter writer;
    LoginSurface s(&writer, temp);

    s.apply(QStringLiteral("/home/test/new-login.jpg"));

    QCOMPARE(writer.installCallCount(), 1);
    QCOMPARE(writer.lastInstallSrc(), QStringLiteral("/home/test/new-login.jpg"));
    QCOMPARE(writer.lastInstallDest(),
             QStringLiteral("/var/lib/plasmalogin/wallpapers/new-login.jpg"));

    QFile::remove(temp);
}

void TestLoginSurface::applyConfPointsAtInstalledCopy()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!temp.isEmpty());

    FakePrivilegedWriter writer;
    LoginSurface s(&writer, temp);
    s.apply(QStringLiteral("/home/test/new-login.jpg"));

    QCOMPARE(writer.callCount(), 1);
    QCOMPARE(writer.lastPath(), temp);
    // The bytes handed to the writefile action must reference the
    // installed copy (plasmalogin-readable), NOT the original $HOME
    // path. That's the entire point of the fix.
    QVERIFY(writer.lastContents().contains(
        "file:///var/lib/plasmalogin/wallpapers/new-login.jpg"));
    QVERIFY(!writer.lastContents().contains("/home/test/new-login.jpg"));
    QVERIFY(writer.lastContents().contains("[Greeter]"));

    QFile::remove(temp);
}

void TestLoginSurface::applyEmitsSucceededAndUpdatesFile()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!temp.isEmpty());

    FakePrivilegedWriter writer;
    LoginSurface s(&writer, temp);
    QSignalSpy success(&s, &WallpaperSurface::applySucceeded);
    QSignalSpy failed(&s, &WallpaperSurface::applyFailed);
    QSignalSpy changed(&s, &WallpaperSurface::currentImagePathChanged);

    s.apply(QStringLiteral("/home/test/new-login.jpg"));

    QCOMPARE(success.count(), 1);
    QCOMPARE(failed.count(), 0);
    QCOMPARE(changed.count(), 1);

    // Re-read with a fresh surface to prove the writer's bytes parse
    // back, and that the recorded image points at the installed copy.
    FakePrivilegedWriter writer2;
    LoginSurface reread(&writer2, temp);
    QCOMPARE(reread.currentImagePath(),
             QStringLiteral("file:///var/lib/plasmalogin/wallpapers/new-login.jpg"));

    QFile::remove(temp);
}

void TestLoginSurface::applyKeepsBasenameFromFileUrl()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!temp.isEmpty());

    FakePrivilegedWriter writer;
    LoginSurface s(&writer, temp);
    s.apply(QStringLiteral("file:///already/prefixed.png"));

    QCOMPARE(writer.lastInstallSrc(), QStringLiteral("/already/prefixed.png"));
    QCOMPARE(writer.lastInstallDest(),
             QStringLiteral("/var/lib/plasmalogin/wallpapers/prefixed.png"));
    QCOMPARE(s.currentImagePath(),
             QStringLiteral("file:///var/lib/plasmalogin/wallpapers/prefixed.png"));
    QFile::remove(temp);
}

void TestLoginSurface::applyEmitsFailedWhenInstallFails()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!temp.isEmpty());

    FakePrivilegedWriter writer;
    writer.setInstallFailureReason(QStringLiteral("polkit prompt cancelled"));
    LoginSurface s(&writer, temp);

    QSignalSpy success(&s, &WallpaperSurface::applySucceeded);
    QSignalSpy failed(&s, &WallpaperSurface::applyFailed);

    s.apply(QStringLiteral("/home/test/never-applied.jpg"));

    QCOMPARE(success.count(), 0);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(failed.first().at(0).toString(), QStringLiteral("polkit prompt cancelled"));

    QFile::remove(temp);
}

void TestLoginSurface::applyDoesNotWriteConfWhenInstallFails()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!temp.isEmpty());

    FakePrivilegedWriter writer;
    writer.setInstallFailureReason(QStringLiteral("install rejected"));
    LoginSurface s(&writer, temp);

    s.apply(QStringLiteral("/home/test/x.jpg"));

    // Install was attempted but the conf-write step must NOT run, or
    // we'd leave /etc/plasmalogin.conf pointing at a file the greeter
    // can't read (the original bug).
    QCOMPARE(writer.installCallCount(), 1);
    QCOMPARE(writer.callCount(), 0);

    QFile::remove(temp);
}

void TestLoginSurface::applyEmitsFailedWhenWriteFails()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!temp.isEmpty());

    FakePrivilegedWriter writer;
    writer.setFailureReason(QStringLiteral("conf write rejected"));
    LoginSurface s(&writer, temp);

    QSignalSpy success(&s, &WallpaperSurface::applySucceeded);
    QSignalSpy failed(&s, &WallpaperSurface::applyFailed);

    s.apply(QStringLiteral("/home/test/x.jpg"));

    QCOMPARE(writer.installCallCount(), 1);
    QCOMPARE(writer.callCount(), 1);
    QCOMPARE(success.count(), 0);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(failed.first().at(0).toString(), QStringLiteral("conf write rejected"));

    QFile::remove(temp);
}

void TestLoginSurface::applyWorksWhenSourceFileMissing()
{
    // A fresh box may have no /etc/plasmalogin.conf override yet.
    // The apply path should still produce a usable file for the writer.
    const QString temp = QDir::temp().filePath(QStringLiteral("pws-test-fresh-plasmalogin.conf"));
    QFile::remove(temp);

    FakePrivilegedWriter writer;
    LoginSurface s(&writer, temp);

    QSignalSpy success(&s, &WallpaperSurface::applySucceeded);
    s.apply(QStringLiteral("/home/test/first.png"));

    QCOMPARE(success.count(), 1);
    QVERIFY(writer.lastContents().contains(
        "file:///var/lib/plasmalogin/wallpapers/first.png"));

    QFile::remove(temp);
}

void TestLoginSurface::readPathAndWritePathCanDiffer()
{
    // Flatpak case: the surface reads from /run/host/etc/... but the
    // writer is told to write to /etc/... — that's the path the
    // privileged helper validates against on the host. Verify the
    // writer sees the write path, not the read path.
    const QString readPath = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!readPath.isEmpty());
    // Per-test temp file rather than /etc/plasmalogin.conf — the
    // FakePrivilegedWriter writes as the test user, which can't touch
    // /etc on a real install. The point of the test is the surface's
    // path-routing behavior, not the writer's actual filesystem reach.
    const QString writePath = QDir::temp().filePath(
        QStringLiteral("pws-test-write-target-plasmalogin.conf"));
    QFile::remove(writePath);

    FakePrivilegedWriter writer;
    LoginSurface s(&writer, readPath, writePath);

    // Read still works through the read path.
    QCOMPARE(s.currentImagePath(),
             QStringLiteral("file:///var/lib/plasmalogin/wallpapers/initial-d-ae86.jpg"));

    QSignalSpy success(&s, &WallpaperSurface::applySucceeded);
    s.apply(QStringLiteral("/home/test/some.jpg"));

    QCOMPARE(writer.callCount(), 1);
    QCOMPARE(writer.lastPath(), writePath);

    // Signal-forwarding filter keys on the write path, so the success
    // signal propagated through too.
    QCOMPARE(success.count(), 1);

    QFile::remove(readPath);
    QFile::remove(writePath);
}

void TestLoginSurface::previewFallsBackToCurrentWhenNoCache()
{
    FakePrivilegedWriter writer;
    LoginSurface s(&writer, fixturePath(QStringLiteral("sample-plasmalogin.conf")));
    // Point the cache at a guaranteed-missing file so a leftover from
    // a previous run can't masquerade as a fresh-state result.
    const QString emptyCache = QDir::temp().filePath(
        QStringLiteral("pws-test-empty-preview-cache.json"));
    QFile::remove(emptyCache);
    s.setPreviewSourceCachePath(emptyCache);

    QCOMPARE(s.previewImagePath(), s.currentImagePath());
}

void TestLoginSurface::previewUsesCachedSourceAfterApply()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!temp.isEmpty());
    const QString cachePath = QDir::temp().filePath(
        QStringLiteral("pws-test-preview-cache.json"));
    QFile::remove(cachePath);

    FakePrivilegedWriter writer;
    LoginSurface s(&writer, temp);
    s.setPreviewSourceCachePath(cachePath);

    s.apply(QStringLiteral("/home/test/picked.jpg"));

    // After apply, the conf points at the installed copy but preview
    // should return the original $HOME source so QML can render it.
    QCOMPARE(s.currentImagePath(),
             QStringLiteral("file:///var/lib/plasmalogin/wallpapers/picked.jpg"));
    QCOMPARE(s.previewImagePath(), QStringLiteral("file:///home/test/picked.jpg"));

    // Cache survives across surface instances (cold-start case).
    LoginSurface reread(&writer, temp);
    reread.setPreviewSourceCachePath(cachePath);
    QCOMPARE(reread.previewImagePath(), QStringLiteral("file:///home/test/picked.jpg"));

    QFile::remove(temp);
    QFile::remove(cachePath);
}

void TestLoginSurface::previewFallsBackWhenCachedDestNoLongerMatches()
{
    // Simulate the case where the user (or another tool) edited
    // /etc/plasmalogin.conf out from under us between two applies.
    // Our cache is keyed by installed-dest, so a conf mismatch is
    // the only signal that the cache is stale.
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!temp.isEmpty());
    const QString cachePath = QDir::temp().filePath(
        QStringLiteral("pws-test-preview-cache-stale.json"));
    QFile::remove(cachePath);

    FakePrivilegedWriter writer;
    LoginSurface s(&writer, temp);
    s.setPreviewSourceCachePath(cachePath);
    s.apply(QStringLiteral("/home/test/old.jpg"));

    // External rewrite: clobber the conf with a different Image= path.
    QFile confFile(temp);
    QVERIFY(confFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    confFile.write(
        "[Greeter][Wallpaper][org.kde.image][General]\n"
        "Image=file:///var/lib/plasmalogin/wallpapers/different.jpg\n");
    confFile.close();

    LoginSurface reread(&writer, temp);
    reread.setPreviewSourceCachePath(cachePath);
    // The stale cache must NOT be returned for the new path; we fall
    // back to the canonical path (which will fail to load in QML, but
    // that's an honest signal that the wallpaper changed externally).
    QCOMPARE(reread.previewImagePath(),
             QStringLiteral("file:///var/lib/plasmalogin/wallpapers/different.jpg"));

    QFile::remove(temp);
    QFile::remove(cachePath);
}

void TestLoginSurface::previewSourceCacheNotWrittenWhenInstallFails()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!temp.isEmpty());
    const QString cachePath = QDir::temp().filePath(
        QStringLiteral("pws-test-preview-cache-noinstall.json"));
    QFile::remove(cachePath);

    FakePrivilegedWriter writer;
    writer.setInstallFailureReason(QStringLiteral("install rejected"));
    LoginSurface s(&writer, temp);
    s.setPreviewSourceCachePath(cachePath);

    s.apply(QStringLiteral("/home/test/never.jpg"));

    // Apply failed at the install step → no conf write happened, so
    // no source mapping should have been persisted either. Otherwise
    // a future fresh launch would render a preview for a wallpaper
    // that was never actually applied.
    QVERIFY(!QFile::exists(cachePath));

    QFile::remove(temp);
}

QString TestLoginSurface::copyFixtureToTemp(const QString &fixtureName)
{
    const QString src = fixturePath(fixtureName);
    const QString dst = QDir::temp().filePath(QStringLiteral("pws-test-") + fixtureName);
    QFile::remove(dst);
    if (!QFile::copy(src, dst)) {
        return {};
    }
    return dst;
}

QString TestLoginSurface::fixturePath(const QString &fixtureName)
{
    return QStringLiteral(PWS_TEST_FIXTURES_DIR "/") + fixtureName;
}

QTEST_GUILESS_MAIN(TestLoginSurface)
#include "tst_LoginSurface.moc"
