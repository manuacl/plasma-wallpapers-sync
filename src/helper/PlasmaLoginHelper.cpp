// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PlasmaLoginHelper.h"

#include <KAuth/HelperSupport>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QString>

#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace
{
const QString kAllowedPath = QStringLiteral("/etc/plasmalogin.conf");
const QString kWallpapersDir = QStringLiteral("/var/lib/plasmalogin/wallpapers");
const QString kPlasmaloginUser = QStringLiteral("plasmalogin");
// Wallpapers are images; 64 MB covers even uncompressed 4K originals
// with plenty of margin. We reject anything larger to refuse using
// this helper as a "copy arbitrary multi-GB blob" primitive.
constexpr qint64 kMaxWallpaperBytes = 64LL * 1024 * 1024;
}

KAuth::ActionReply PlasmaLoginHelper::writefile(const QVariantMap &args)
{
    const QString path = args.value(QStringLiteral("path")).toString();
    const QByteArray contents = args.value(QStringLiteral("contents")).toByteArray();

    // Defense in depth — the polkit policy is the primary gate, but
    // we refuse any path other than /etc/plasmalogin.conf here too
    // so a misconfigured policy can never escalate this helper into
    // a "write anywhere as root" primitive.
    if (path != kAllowedPath) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(
            QStringLiteral("Refusing to write to %1; only %2 is permitted")
                .arg(path, kAllowedPath));
        return reply;
    }

    // QSaveFile writes to a sibling temp file and atomically renames
    // on commit() — keeps the original intact on any partial-write
    // failure.
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(file.errorString());
        return reply;
    }

    if (file.write(contents) != contents.size()) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(file.errorString());
        return reply;
    }

    if (!file.commit()) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(file.errorString());
        return reply;
    }

    return KAuth::ActionReply::SuccessReply();
}

KAuth::ActionReply PlasmaLoginHelper::installwallpaper(const QVariantMap &args)
{
    const QString src = args.value(QStringLiteral("src")).toString();
    const QString dest = args.value(QStringLiteral("dest")).toString();

    // Dest must be /var/lib/plasmalogin/wallpapers/<basename> with a
    // safe basename. QDir::cleanPath collapses .. and duplicate
    // slashes; we then require the cleaned dest to live directly
    // inside the wallpapers dir, and the basename to round-trip
    // through QFileInfo without surprises.
    const QString cleanedDest = QDir::cleanPath(dest);
    const QString destDir = QFileInfo(cleanedDest).absolutePath();
    const QString destBasename = QFileInfo(cleanedDest).fileName();
    if (destDir != kWallpapersDir
        || destBasename.isEmpty()
        || destBasename.contains(QLatin1Char('/'))
        || destBasename == QStringLiteral(".")
        || destBasename == QStringLiteral("..")) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(
            QStringLiteral("Refusing install to %1; only files directly under %2 are permitted")
                .arg(dest, kWallpapersDir));
        return reply;
    }

    QFileInfo srcInfo(src);
    if (!srcInfo.isFile() || !srcInfo.isReadable()) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(
            QStringLiteral("Source %1 is not a readable file").arg(src));
        return reply;
    }
    if (srcInfo.size() > kMaxWallpaperBytes) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(
            QStringLiteral("Source %1 is %2 bytes, larger than the %3-byte cap")
                .arg(src)
                .arg(srcInfo.size())
                .arg(kMaxWallpaperBytes));
        return reply;
    }

    // Make sure the target directory exists. On a vanilla plasmalogin
    // install /var/lib/plasmalogin/wallpapers/ is already there with
    // plasmalogin ownership; we mkdir defensively in case a future
    // setup ships without it.
    QDir().mkpath(kWallpapersDir);

    QFile srcFile(src);
    if (!srcFile.open(QIODevice::ReadOnly)) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(srcFile.errorString());
        return reply;
    }
    const QByteArray bytes = srcFile.readAll();
    srcFile.close();

    QSaveFile out(cleanedDest);
    if (!out.open(QIODevice::WriteOnly)) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(out.errorString());
        return reply;
    }
    if (out.write(bytes) != bytes.size()) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(out.errorString());
        return reply;
    }
    if (!out.commit()) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(out.errorString());
        return reply;
    }

    // chown/chmod after the rename so the plasmalogin user can read
    // the result. Failing chown is fatal: a root-owned 0600 file in
    // /var/lib/plasmalogin/wallpapers/ would silently break the
    // greeter the same way the original $HOME path did.
    const QByteArray destBytes = cleanedDest.toLocal8Bit();
    if (::chmod(destBytes.constData(), 0644) != 0) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(
            QStringLiteral("chmod 0644 %1 failed: %2").arg(cleanedDest, QString::fromLocal8Bit(::strerror(errno))));
        return reply;
    }
    const struct passwd *pw = ::getpwnam(kPlasmaloginUser.toLocal8Bit().constData());
    if (!pw) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(
            QStringLiteral("Unknown user %1 — is plasmalogin installed?").arg(kPlasmaloginUser));
        return reply;
    }
    if (::chown(destBytes.constData(), pw->pw_uid, pw->pw_gid) != 0) {
        KAuth::ActionReply reply = KAuth::ActionReply::HelperErrorReply();
        reply.setErrorDescription(
            QStringLiteral("chown %1:%1 %2 failed: %3")
                .arg(kPlasmaloginUser, cleanedDest, QString::fromLocal8Bit(::strerror(errno))));
        return reply;
    }

    return KAuth::ActionReply::SuccessReply();
}

KAUTH_HELPER_MAIN("dev.manuacl.plasmawallpapersync", PlasmaLoginHelper)
