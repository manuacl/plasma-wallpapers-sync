// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "WallpaperLibrary.h"

#include <QDir>
#include <QFile>
#include <QObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

class TestWallpaperLibrary : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void emptyForEmptySearchPath();
    void picksPackageWallpapersBySubdirName();
    void picksFlatImageFiles();
    void rolesExposeNamePreviewApplyAsFileUrls();
    void duplicateNamesAreDeduplicated();
    void reloadPicksUpNewlyAddedPackage();
    void picksFlatImagesInsideOneLevelSubdir();

private:
    static void createPackage(const QString &root, const QString &name, const QString &imageFile);
    static void createFlatImage(const QString &root, const QString &filename);
};

void TestWallpaperLibrary::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void TestWallpaperLibrary::emptyForEmptySearchPath()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    WallpaperLibrary lib;
    lib.setSearchPaths({tmp.path()});

    QCOMPARE(lib.rowCount(), 0);
}

void TestWallpaperLibrary::picksPackageWallpapersBySubdirName()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    createPackage(tmp.path(), QStringLiteral("Hawk"), QStringLiteral("1920x1080.png"));
    createPackage(tmp.path(), QStringLiteral("Patak"), QStringLiteral("2880x1800.jpg"));

    WallpaperLibrary lib;
    lib.setSearchPaths({tmp.path()});

    QCOMPARE(lib.rowCount(), 2);

    // Names are subdir names, alphabetical order.
    QCOMPARE(lib.data(lib.index(0), WallpaperLibrary::NameRole).toString(),
             QStringLiteral("Hawk"));
    QCOMPARE(lib.data(lib.index(1), WallpaperLibrary::NameRole).toString(),
             QStringLiteral("Patak"));
}

void TestWallpaperLibrary::picksFlatImageFiles()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    createFlatImage(tmp.path(), QStringLiteral("sunset.jpg"));
    createFlatImage(tmp.path(), QStringLiteral("forest.webp"));

    WallpaperLibrary lib;
    lib.setSearchPaths({tmp.path()});

    QCOMPARE(lib.rowCount(), 2);

    // entryList(Name) sorts alphabetically: forest < sunset.
    QCOMPARE(lib.data(lib.index(0), WallpaperLibrary::NameRole).toString(),
             QStringLiteral("forest"));
    QCOMPARE(lib.data(lib.index(1), WallpaperLibrary::NameRole).toString(),
             QStringLiteral("sunset"));
}

void TestWallpaperLibrary::rolesExposeNamePreviewApplyAsFileUrls()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    createPackage(tmp.path(), QStringLiteral("Hawk"), QStringLiteral("1920x1080.png"));

    WallpaperLibrary lib;
    lib.setSearchPaths({tmp.path()});

    QCOMPARE(lib.rowCount(), 1);
    const auto idx = lib.index(0);

    QCOMPARE(lib.data(idx, WallpaperLibrary::NameRole).toString(),
             QStringLiteral("Hawk"));

    const QString expected = QUrl::fromLocalFile(tmp.path()
        + QStringLiteral("/Hawk/contents/images/1920x1080.png")).toString();
    QCOMPARE(lib.data(idx, WallpaperLibrary::PreviewPathRole).toString(), expected);
    QCOMPARE(lib.data(idx, WallpaperLibrary::ApplyPathRole).toString(), expected);

    // roleNames must match the strings QML's required-property syntax expects.
    const auto roles = lib.roleNames();
    QCOMPARE(roles.value(WallpaperLibrary::NameRole), QByteArrayLiteral("name"));
    QCOMPARE(roles.value(WallpaperLibrary::PreviewPathRole), QByteArrayLiteral("previewPath"));
    QCOMPARE(roles.value(WallpaperLibrary::ApplyPathRole), QByteArrayLiteral("applyPath"));
}

void TestWallpaperLibrary::duplicateNamesAreDeduplicated()
{
    // Same package name in two search paths — the first one wins so
    // a user-local override doesn't surface twice when the system
    // dir also ships the same theme.
    QTemporaryDir userDir;
    QTemporaryDir systemDir;
    QVERIFY(userDir.isValid());
    QVERIFY(systemDir.isValid());

    createPackage(userDir.path(), QStringLiteral("Hawk"), QStringLiteral("user.png"));
    createPackage(systemDir.path(), QStringLiteral("Hawk"), QStringLiteral("system.png"));

    WallpaperLibrary lib;
    lib.setSearchPaths({userDir.path(), systemDir.path()});

    QCOMPARE(lib.rowCount(), 1);
    const QString preview = lib.data(lib.index(0), WallpaperLibrary::PreviewPathRole).toString();
    QVERIFY(preview.contains(QStringLiteral("user.png")));
    QVERIFY(!preview.contains(QStringLiteral("system.png")));
}

void TestWallpaperLibrary::picksFlatImagesInsideOneLevelSubdir()
{
    // ~/.local/share/wallpapers/<topic>/<image>.<ext> is the common
    // "I dumped images in a folder under the wallpapers dir" shape —
    // not a KPackage structure, just regular files. Each image
    // surfaces as its own entry; the parent folder isn't shown.
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    QVERIFY(QDir(tmp.path()).mkpath(QStringLiteral("Personal")));
    QFile a(tmp.path() + QStringLiteral("/Personal/sunset.jpg"));
    QVERIFY(a.open(QIODevice::WriteOnly));
    a.close();
    QFile b(tmp.path() + QStringLiteral("/Personal/forest.png"));
    QVERIFY(b.open(QIODevice::WriteOnly));
    b.close();

    WallpaperLibrary lib;
    lib.setSearchPaths({tmp.path()});

    QCOMPARE(lib.rowCount(), 2);
    QStringList names;
    for (int i = 0; i < lib.rowCount(); ++i) {
        names << lib.data(lib.index(i), WallpaperLibrary::NameRole).toString();
    }
    names.sort();
    QCOMPARE(names, QStringList({QStringLiteral("forest"), QStringLiteral("sunset")}));
}

void TestWallpaperLibrary::reloadPicksUpNewlyAddedPackage()
{
    QTemporaryDir tmp;
    QVERIFY(tmp.isValid());

    WallpaperLibrary lib;
    lib.setSearchPaths({tmp.path()});
    QCOMPARE(lib.rowCount(), 0);

    createPackage(tmp.path(), QStringLiteral("LateArrival"), QStringLiteral("4k.png"));
    lib.reload();

    QCOMPARE(lib.rowCount(), 1);
    QCOMPARE(lib.data(lib.index(0), WallpaperLibrary::NameRole).toString(),
             QStringLiteral("LateArrival"));
}

void TestWallpaperLibrary::createPackage(const QString &root,
                                          const QString &name,
                                          const QString &imageFile)
{
    QDir dir(root);
    QVERIFY(dir.mkpath(name + QStringLiteral("/contents/images")));
    QFile f(root + QLatin1Char('/') + name
            + QStringLiteral("/contents/images/") + imageFile);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.close();
}

void TestWallpaperLibrary::createFlatImage(const QString &root, const QString &filename)
{
    QFile f(root + QLatin1Char('/') + filename);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.close();
}

QTEST_GUILESS_MAIN(TestWallpaperLibrary)
#include "tst_WallpaperLibrary.moc"
