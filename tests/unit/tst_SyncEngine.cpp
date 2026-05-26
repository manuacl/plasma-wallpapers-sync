// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "FakeSurface.h"
#include "SyncEngine.h"

#include <QObject>
#include <QSignalSpy>
#include <QTest>

class TestSyncEngine : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void registrationAndLookup();
    void addSurfaceIgnoresNullAndDuplicates();
    void surfaceIdsPreservesInsertionOrder();
    void applyToAllInvokesEverySurface();
    void applyToSurfacesInvokesOnlyTargets();
    void emptyTargetEmitsFinishedImmediately();
    void unknownTargetIdsAreSilentlyDropped();
    void applyFinishedReportsSuccessesAndFailures();
    void perSurfaceSignalsAreForwarded();
    void overlappingApplyIsIgnored();
    void isApplyingFlipsAroundInFlightBatch();
    void isApplyingStaysFalseOnEmptyBatch();
    void isApplyingStillFlipsWithSyncSurfaces();
};

void TestSyncEngine::registrationAndLookup()
{
    SyncEngine engine;
    FakeSurface desktop(QStringLiteral("desktop"));
    engine.addSurface(&desktop);

    QCOMPARE(engine.surface(QStringLiteral("desktop")), &desktop);
    QCOMPARE(engine.surface(QStringLiteral("nope")), static_cast<WallpaperSurface *>(nullptr));
    QVERIFY(!engine.isApplying());
}

void TestSyncEngine::addSurfaceIgnoresNullAndDuplicates()
{
    SyncEngine engine;
    FakeSurface desktop(QStringLiteral("desktop"));
    FakeSurface dupe(QStringLiteral("desktop"));

    engine.addSurface(nullptr);
    engine.addSurface(&desktop);
    engine.addSurface(&dupe);

    QCOMPARE(engine.surfaceIds(), QStringList{QStringLiteral("desktop")});
    QCOMPARE(engine.surface(QStringLiteral("desktop")), &desktop);
}

void TestSyncEngine::surfaceIdsPreservesInsertionOrder()
{
    SyncEngine engine;
    FakeSurface desktop(QStringLiteral("desktop"));
    FakeSurface lock(QStringLiteral("lockscreen"));
    FakeSurface login(QStringLiteral("login"));

    engine.addSurface(&desktop);
    engine.addSurface(&lock);
    engine.addSurface(&login);

    QCOMPARE(engine.surfaceIds(),
             QStringList({QStringLiteral("desktop"),
                          QStringLiteral("lockscreen"),
                          QStringLiteral("login")}));
}

void TestSyncEngine::applyToAllInvokesEverySurface()
{
    SyncEngine engine;
    FakeSurface a(QStringLiteral("a"));
    FakeSurface b(QStringLiteral("b"));
    engine.addSurface(&a);
    engine.addSurface(&b);

    QSignalSpy finished(&engine, &SyncEngine::applyFinished);
    engine.applyToAll(QStringLiteral("/img.png"));

    QCOMPARE(a.applyCallCount(), 1);
    QCOMPARE(b.applyCallCount(), 1);
    QCOMPARE(a.lastApplyPath(), QStringLiteral("/img.png"));
    QCOMPARE(finished.count(), 1);
    const auto succeeded = finished.first().at(0).toStringList();
    QCOMPARE(succeeded, QStringList({QStringLiteral("a"), QStringLiteral("b")}));
    QCOMPARE(finished.first().at(1).toStringList(), QStringList());
}

void TestSyncEngine::applyToSurfacesInvokesOnlyTargets()
{
    SyncEngine engine;
    FakeSurface a(QStringLiteral("a"));
    FakeSurface b(QStringLiteral("b"));
    FakeSurface c(QStringLiteral("c"));
    engine.addSurface(&a);
    engine.addSurface(&b);
    engine.addSurface(&c);

    QSignalSpy finished(&engine, &SyncEngine::applyFinished);
    engine.applyToSurfaces(QStringLiteral("/x.png"),
                           {QStringLiteral("a"), QStringLiteral("c")});

    QCOMPARE(a.applyCallCount(), 1);
    QCOMPARE(b.applyCallCount(), 0);
    QCOMPARE(c.applyCallCount(), 1);
    QCOMPARE(finished.count(), 1);
    QCOMPARE(finished.first().at(0).toStringList(),
             QStringList({QStringLiteral("a"), QStringLiteral("c")}));
}

void TestSyncEngine::emptyTargetEmitsFinishedImmediately()
{
    SyncEngine engine;
    FakeSurface a(QStringLiteral("a"));
    engine.addSurface(&a);

    QSignalSpy finished(&engine, &SyncEngine::applyFinished);
    engine.applyToSurfaces(QStringLiteral("/whatever"), {});

    QCOMPARE(a.applyCallCount(), 0);
    QCOMPARE(finished.count(), 1);
    QCOMPARE(finished.first().at(0).toStringList(), QStringList());
    QCOMPARE(finished.first().at(1).toStringList(), QStringList());
}

void TestSyncEngine::unknownTargetIdsAreSilentlyDropped()
{
    SyncEngine engine;
    FakeSurface a(QStringLiteral("a"));
    engine.addSurface(&a);

    QSignalSpy finished(&engine, &SyncEngine::applyFinished);
    engine.applyToSurfaces(QStringLiteral("/x"),
                           {QStringLiteral("a"), QStringLiteral("ghost")});

    QCOMPARE(a.applyCallCount(), 1);
    QCOMPARE(finished.count(), 1);
    QCOMPARE(finished.first().at(0).toStringList(),
             QStringList{QStringLiteral("a")});
    QCOMPARE(finished.first().at(1).toStringList(), QStringList());
}

void TestSyncEngine::applyFinishedReportsSuccessesAndFailures()
{
    SyncEngine engine;
    FakeSurface ok(QStringLiteral("ok"));
    FakeSurface broken(QStringLiteral("broken"));
    broken.setFailureReason(QStringLiteral("polkit cancelled"));
    engine.addSurface(&ok);
    engine.addSurface(&broken);

    QSignalSpy finished(&engine, &SyncEngine::applyFinished);
    engine.applyToAll(QStringLiteral("/img"));

    QCOMPARE(finished.count(), 1);
    QCOMPARE(finished.first().at(0).toStringList(),
             QStringList{QStringLiteral("ok")});
    QCOMPARE(finished.first().at(1).toStringList(),
             QStringList{QStringLiteral("broken")});
}

void TestSyncEngine::perSurfaceSignalsAreForwarded()
{
    SyncEngine engine;
    FakeSurface ok(QStringLiteral("ok"));
    FakeSurface broken(QStringLiteral("broken"));
    broken.setFailureReason(QStringLiteral("nope"));
    engine.addSurface(&ok);
    engine.addSurface(&broken);

    QSignalSpy okSpy(&engine, &SyncEngine::surfaceApplySucceeded);
    QSignalSpy failSpy(&engine, &SyncEngine::surfaceApplyFailed);

    engine.applyToAll(QStringLiteral("/img"));

    QCOMPARE(okSpy.count(), 1);
    QCOMPARE(okSpy.first().at(0).toString(), QStringLiteral("ok"));
    QCOMPARE(failSpy.count(), 1);
    QCOMPARE(failSpy.first().at(0).toString(), QStringLiteral("broken"));
    QCOMPARE(failSpy.first().at(1).toString(), QStringLiteral("nope"));
}

void TestSyncEngine::overlappingApplyIsIgnored()
{
    // Build a surface that does NOT auto-emit so we can keep the
    // batch in flight while we issue a second apply.
    class StuckSurface : public WallpaperSurface
    {
    public:
        QString id() const override { return QStringLiteral("stuck"); }
        QString displayName() const override { return QStringLiteral("Stuck"); }
        QString currentImagePath() const override { return {}; }
        void apply(const QString &) override { /* never emits */ }
    } stuck;

    SyncEngine engine;
    engine.addSurface(&stuck);

    QSignalSpy finished(&engine, &SyncEngine::applyFinished);

    engine.applyToAll(QStringLiteral("/first"));
    QVERIFY(engine.isApplying());
    QCOMPARE(finished.count(), 0);

    // Second call must be a no-op — neither dispatches nor emits.
    engine.applyToAll(QStringLiteral("/second"));
    QCOMPARE(finished.count(), 0);
    QVERIFY(engine.isApplying());
}

namespace
{
class StuckSurface : public WallpaperSurface
{
public:
    explicit StuckSurface(const QString &id) : m_id(id) {}
    QString id() const override { return m_id; }
    QString displayName() const override { return m_id; }
    QString currentImagePath() const override { return {}; }
    void apply(const QString &) override { /* never emits — batch stays in flight */ }
private:
    QString m_id;
};
}

void TestSyncEngine::isApplyingFlipsAroundInFlightBatch()
{
    StuckSurface stuck(QStringLiteral("stuck"));
    SyncEngine engine;
    engine.addSurface(&stuck);

    QSignalSpy applyingSpy(&engine, &SyncEngine::applyingChanged);
    QVERIFY(!engine.isApplying());

    engine.applyToAll(QStringLiteral("/x"));
    QVERIFY(engine.isApplying());
    QCOMPARE(applyingSpy.count(), 1);
}

void TestSyncEngine::isApplyingStaysFalseOnEmptyBatch()
{
    SyncEngine engine;
    QSignalSpy applyingSpy(&engine, &SyncEngine::applyingChanged);

    engine.applyToSurfaces(QStringLiteral("/x"), {});
    QVERIFY(!engine.isApplying());
    QCOMPARE(applyingSpy.count(), 0);
}

void TestSyncEngine::isApplyingStillFlipsWithSyncSurfaces()
{
    // Even when every surface reports synchronously, the engine emits
    // both transitions (false→true→false) so the GUI's "Apply disabled
    // while applying" binding is reactive.
    FakeSurface a(QStringLiteral("a"));
    FakeSurface b(QStringLiteral("b"));
    SyncEngine engine;
    engine.addSurface(&a);
    engine.addSurface(&b);

    QSignalSpy applyingSpy(&engine, &SyncEngine::applyingChanged);
    engine.applyToAll(QStringLiteral("/x"));

    QVERIFY(!engine.isApplying());
    QCOMPARE(applyingSpy.count(), 2);
}

QTEST_GUILESS_MAIN(TestSyncEngine)
#include "tst_SyncEngine.moc"
