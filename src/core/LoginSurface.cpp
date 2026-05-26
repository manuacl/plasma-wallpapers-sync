// SPDX-FileCopyrightText: 2026 Manuel Chamorro <manu.acl@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LoginSurface.h"

#include "PrivilegedWriter.h"

#include <KConfig>
#include <KConfigGroup>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QUrl>

namespace
{
const QString kGreeter = QStringLiteral("Greeter");
const QString kWallpaper = QStringLiteral("Wallpaper");
const QString kImagePlugin = QStringLiteral("org.kde.image");
const QString kGeneral = QStringLiteral("General");
const QString kImageKey = QStringLiteral("Image");
const QString kHostConfigPath = QStringLiteral("/etc/plasmalogin.conf");
const QString kFlatpakInfoMarker = QStringLiteral("/.flatpak-info");
const QString kFlatpakHostReadPath = QStringLiteral("/run/host/etc/plasmalogin.conf");
const QString kPlasmaloginWallpapersDir = QStringLiteral("/var/lib/plasmalogin/wallpapers");
const QString kPreviewCacheFile = QStringLiteral("login-preview-source.json");
const QString kJsonDestKey = QStringLiteral("installedDest");
const QString kJsonSourceKey = QStringLiteral("sourceUrl");
}

LoginSurface::LoginSurface(PrivilegedWriter *writer, QObject *parent)
    : LoginSurface(writer, defaultReadPath(), defaultWritePath(), parent)
{
}

LoginSurface::LoginSurface(PrivilegedWriter *writer, const QString &configPath, QObject *parent)
    : LoginSurface(writer, configPath, configPath, parent)
{
}

LoginSurface::LoginSurface(PrivilegedWriter *writer,
                            const QString &readPath,
                            const QString &writePath,
                            QObject *parent)
    : WallpaperSurface(parent)
    , m_writer(writer)
    , m_readPath(readPath)
    , m_writePath(writePath)
{
    wireWriterSignals();
}

LoginSurface::~LoginSurface() = default;

QString LoginSurface::id() const
{
    return QStringLiteral("login");
}

QString LoginSurface::displayName() const
{
    return tr("Login screen");
}

QString LoginSurface::description() const
{
    return tr("Shown by plasmalogin before any user logs in, behind the user list and password prompt.");
}

QString LoginSurface::defaultReadPath()
{
    // Native install: /etc/plasmalogin.conf is the host's file.
    // Flatpak: /etc/ inside the sandbox is the runtime's /etc, NOT
    // the host's. With --filesystem=host:ro the host's /etc/ is
    // bind-mounted at /run/host/etc/. Detect Flatpak via the
    // sentinel /.flatpak-info and route the read there.
    if (QFileInfo::exists(kFlatpakInfoMarker)) {
        return kFlatpakHostReadPath;
    }
    return kHostConfigPath;
}

QString LoginSurface::defaultWritePath()
{
    // The privileged helper runs on the host (or, under Flatpak,
    // would-run via system D-Bus + polkit) and validates the path it
    // writes to against the canonical /etc/plasmalogin.conf. So the
    // write path is always the host-canonical path, regardless of
    // where the client reads from.
    return kHostConfigPath;
}

QString LoginSurface::sanitizeBasename(const QString &candidate)
{
    // The helper validates the destination path again — this side is
    // for friendlier error messages and to keep the install path tidy
    // in /var/lib/plasmalogin/wallpapers/. We strip anything that
    // isn't [A-Za-z0-9._-]; the empty result is replaced with a
    // generic name so a path-less input still installs cleanly.
    QString out = candidate;
    static const QRegularExpression unsafe(QStringLiteral("[^A-Za-z0-9._-]"));
    out.replace(unsafe, QStringLiteral("_"));
    while (out.startsWith(QLatin1Char('.'))) {
        out.remove(0, 1);
    }
    if (out.isEmpty()) {
        out = QStringLiteral("wallpaper");
    }
    return out;
}

QString LoginSurface::resolveLocalPath(const QString &maybeUrl)
{
    if (maybeUrl.startsWith(QLatin1String("file://"))) {
        return QUrl(maybeUrl).toLocalFile();
    }
    return maybeUrl;
}

QString LoginSurface::currentImagePath() const
{
    KConfig config(m_readPath, KConfig::SimpleConfig);
    const KConfigGroup imageGroup = config.group(kGreeter)
                                        .group(kWallpaper)
                                        .group(kImagePlugin)
                                        .group(kGeneral);
    return imageGroup.readEntry(kImageKey, QString());
}

QString LoginSurface::previewImagePath() const
{
    // The conf path lives under /var/lib/plasmalogin/wallpapers/,
    // which is unreachable from $USER on a stock plasmalogin install
    // (parent dir mode 0750). The cached source URL was readable when
    // the user picked it and is overwhelmingly still readable now —
    // return it so the QML Image element can actually render. If the
    // mapping is missing (cold start before any apply, cache cleared,
    // conf changed externally), fall back to the canonical path: the
    // QML side already has a "Failed to load preview" placeholder for
    // that case.
    const QString conf = currentImagePath();
    if (conf.isEmpty()) {
        return conf;
    }
    const QString cached = readCachedSourceUrlFor(conf);
    return cached.isEmpty() ? conf : cached;
}

void LoginSurface::setPreviewSourceCachePath(const QString &path)
{
    m_overrideCachePath = path;
}

QString LoginSurface::previewSourceCachePath() const
{
    if (!m_overrideCachePath.isEmpty()) {
        return m_overrideCachePath;
    }
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (dir.isEmpty()) {
        return {};
    }
    QDir().mkpath(dir);
    return dir + QLatin1Char('/') + kPreviewCacheFile;
}

void LoginSurface::persistPreviewSourceMapping(const QString &installedDest,
                                                const QString &sourceUrl)
{
    const QString path = previewSourceCachePath();
    if (path.isEmpty()) {
        return; // No cache location available — preview falls back to canonical.
    }
    QJsonObject obj;
    obj.insert(kJsonDestKey, installedDest);
    obj.insert(kJsonSourceKey, sourceUrl);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return; // Best-effort — preview will degrade, apply itself succeeded.
    }
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    f.close();
}

QString LoginSurface::readCachedSourceUrlFor(const QString &installedDest) const
{
    const QString path = previewSourceCachePath();
    if (path.isEmpty() || !QFile::exists(path)) {
        return {};
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        return {};
    }
    const QByteArray bytes = f.readAll();
    f.close();

    const QJsonDocument doc = QJsonDocument::fromJson(bytes);
    if (!doc.isObject()) {
        return {};
    }
    const QJsonObject obj = doc.object();
    // The cache is only valid if the conf still points at the install
    // we recorded — otherwise the user (or another tool) changed the
    // login wallpaper out from under us and our source is stale.
    if (obj.value(kJsonDestKey).toString() != installedDest) {
        return {};
    }
    return obj.value(kJsonSourceKey).toString();
}

void LoginSurface::apply(const QString &imagePath)
{
    if (!m_writer) {
        Q_EMIT applyFailed(tr("No privileged writer configured"));
        return;
    }

    const QString localSrc = resolveLocalPath(imagePath);
    if (localSrc.isEmpty()) {
        Q_EMIT applyFailed(tr("Cannot resolve %1 to a local file").arg(imagePath));
        return;
    }
    const QString basename = sanitizeBasename(QFileInfo(localSrc).fileName());
    m_pendingInstallDest = kPlasmaloginWallpapersDir + QLatin1Char('/') + basename;
    // Remember the user-side URL so the preview can keep showing it
    // after success — /var/lib/plasmalogin/wallpapers/ isn't readable
    // from $USER, but the original source they picked is.
    m_pendingSourceUrl = QUrl::fromLocalFile(localSrc).toString();

    // Hand the (untouched) source path to the writer; the helper
    // running as root will read it and copy into the plasmalogin-
    // owned directory. The conf-write step is chained on install
    // success in wireWriterSignals().
    m_writer->installFile(localSrc, m_pendingInstallDest);
}

void LoginSurface::writeConfWithImage(const QString &installedImageUrl)
{
    // Stage a user-writable copy of the conf so we can mutate it with
    // KConfig without touching the privileged file. If the source
    // doesn't exist (fresh system), we start from an empty staging
    // file.
    QTemporaryFile staging;
    staging.setAutoRemove(true);
    if (!staging.open()) {
        Q_EMIT applyFailed(tr("Cannot create staging file"));
        return;
    }
    const QString stagingPath = staging.fileName();
    staging.close();

    if (QFile::exists(m_readPath)) {
        QFile::remove(stagingPath);
        if (!QFile::copy(m_readPath, stagingPath)) {
            Q_EMIT applyFailed(tr("Cannot stage %1").arg(m_readPath));
            return;
        }
    }

    {
        KConfig config(stagingPath, KConfig::SimpleConfig);
        KConfigGroup imageGroup = config.group(kGreeter)
                                      .group(kWallpaper)
                                      .group(kImagePlugin)
                                      .group(kGeneral);
        imageGroup.writeEntry(kImageKey, installedImageUrl);
        if (!config.sync()) {
            Q_EMIT applyFailed(tr("Cannot stage edit to %1").arg(stagingPath));
            return;
        }
    }

    QFile staged(stagingPath);
    if (!staged.open(QIODevice::ReadOnly)) {
        Q_EMIT applyFailed(tr("Cannot read staged file"));
        return;
    }
    const QByteArray contents = staged.readAll();
    staged.close();

    m_writer->writeAtomically(m_writePath, contents);
}

void LoginSurface::wireWriterSignals()
{
    if (!m_writer) {
        return;
    }
    // Install step success → kick off the conf write pointing at the
    // staged copy. The Image= URL is the installed location, not the
    // original $HOME path, because that's the path the greeter
    // (running as `plasmalogin`) can actually open.
    connect(m_writer, &PrivilegedWriter::installSucceeded,
            this, [this](const QString &destPath) {
                if (destPath != m_pendingInstallDest) {
                    return;
                }
                writeConfWithImage(QUrl::fromLocalFile(destPath).toString());
            });
    connect(m_writer, &PrivilegedWriter::installFailed,
            this, [this](const QString &destPath, const QString &reason) {
                if (destPath != m_pendingInstallDest) {
                    return;
                }
                Q_EMIT applyFailed(reason);
            });
    connect(m_writer, &PrivilegedWriter::writeSucceeded,
            this, [this](const QString &path) {
                if (path != m_writePath) {
                    return;
                }
                // The conf is now committed — record the source URL
                // mapping so subsequent previewImagePath() lookups
                // resolve to the user-readable path. Persisting
                // before the conf write would leave a dangling
                // mapping if the write failed.
                if (!m_pendingInstallDest.isEmpty()
                    && !m_pendingSourceUrl.isEmpty()) {
                    // Key the cache by the URL form, since
                    // currentImagePath() returns what the conf
                    // stored (URL with scheme), and that's what
                    // readCachedSourceUrlFor() compares against.
                    persistPreviewSourceMapping(
                        QUrl::fromLocalFile(m_pendingInstallDest).toString(),
                        m_pendingSourceUrl);
                }
                Q_EMIT currentImagePathChanged();
                Q_EMIT applySucceeded();
            });
    connect(m_writer, &PrivilegedWriter::writeFailed,
            this, [this](const QString &path, const QString &reason) {
                if (path != m_writePath) {
                    return;
                }
                Q_EMIT applyFailed(reason);
            });
}
