// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PlasmaReloader.h"

#include <QObject>
#include <QSignalSpy>
#include <QTest>

/**
 * The reloader is a thin wrapper around QtDBus. We don't try to fake
 * the bus; instead we verify the behavior we can observe in any
 * environment where plasmashell isn't running (the CI sandbox, the
 * build inside org.kde.Sdk, a TTY session):
 *   - notifyDesktopChanged on a host without org.kde.plasmashell
 *     ends with notificationFailed("desktop", reason)
 *   - notifyLockscreenChanged and notifyLoginChanged are silent
 *     no-ops by design (documented in the header)
 *
 * The happy-path integration with a live Plasma session is verified
 * manually on the developer's machine; pushing a fake D-Bus daemon
 * into the unit tests would buy little for a class this thin.
 */
class TestPlasmaReloader : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void desktopWithoutPlasmaShellEmitsFailure();
    void lockscreenIsSilentNoOp();
    void loginIsSilentNoOp();
};

void TestPlasmaReloader::desktopWithoutPlasmaShellEmitsFailure()
{
    PlasmaReloader r;
    QSignalSpy failed(&r, &PlasmaReloader::notificationFailed);

    r.notifyDesktopChanged();

    QCOMPARE(failed.count(), 1);
    QCOMPARE(failed.first().at(0).toString(), QStringLiteral("desktop"));
    QVERIFY(!failed.first().at(1).toString().isEmpty());
}

void TestPlasmaReloader::lockscreenIsSilentNoOp()
{
    PlasmaReloader r;
    QSignalSpy failed(&r, &PlasmaReloader::notificationFailed);

    r.notifyLockscreenChanged();

    QCOMPARE(failed.count(), 0);
}

void TestPlasmaReloader::loginIsSilentNoOp()
{
    PlasmaReloader r;
    QSignalSpy failed(&r, &PlasmaReloader::notificationFailed);

    r.notifyLoginChanged();

    QCOMPARE(failed.count(), 0);
}

QTEST_GUILESS_MAIN(TestPlasmaReloader)
#include "tst_PlasmaReloader.moc"
