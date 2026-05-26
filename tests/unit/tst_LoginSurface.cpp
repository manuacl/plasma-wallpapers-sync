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
    void applyRoutesThroughWriter();
    void applyEmitsSucceededAndUpdatesFile();
    void applyKeepsExistingFileUrlPrefix();
    void applyEmitsFailedWhenWriterFails();
    void applyWorksWhenSourceFileMissing();
    void readPathAndWritePathCanDiffer();

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

void TestLoginSurface::applyRoutesThroughWriter()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!temp.isEmpty());

    FakePrivilegedWriter writer;
    LoginSurface s(&writer, temp);

    s.apply(QStringLiteral("/home/test/new-login.jpg"));

    QCOMPARE(writer.callCount(), 1);
    QCOMPARE(writer.lastPath(), temp);
    QVERIFY(writer.lastContents().contains("file:///home/test/new-login.jpg"));
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

    // Re-read with a fresh surface to prove the writer's bytes parse back.
    FakePrivilegedWriter writer2;
    LoginSurface reread(&writer2, temp);
    QCOMPARE(reread.currentImagePath(),
             QStringLiteral("file:///home/test/new-login.jpg"));

    QFile::remove(temp);
}

void TestLoginSurface::applyKeepsExistingFileUrlPrefix()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!temp.isEmpty());

    FakePrivilegedWriter writer;
    LoginSurface s(&writer, temp);
    s.apply(QStringLiteral("file:///already/prefixed.png"));

    QCOMPARE(s.currentImagePath(), QStringLiteral("file:///already/prefixed.png"));
    QFile::remove(temp);
}

void TestLoginSurface::applyEmitsFailedWhenWriterFails()
{
    const QString temp = copyFixtureToTemp(QStringLiteral("sample-plasmalogin.conf"));
    QVERIFY(!temp.isEmpty());

    FakePrivilegedWriter writer;
    writer.setFailureReason(QStringLiteral("polkit prompt cancelled"));
    LoginSurface s(&writer, temp);

    QSignalSpy success(&s, &WallpaperSurface::applySucceeded);
    QSignalSpy failed(&s, &WallpaperSurface::applyFailed);

    s.apply(QStringLiteral("/home/test/never-applied.jpg"));

    QCOMPARE(success.count(), 0);
    QCOMPARE(failed.count(), 1);
    QCOMPARE(failed.first().at(0).toString(), QStringLiteral("polkit prompt cancelled"));

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
    QVERIFY(writer.lastContents().contains("file:///home/test/first.png"));

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
