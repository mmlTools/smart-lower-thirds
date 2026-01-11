// api.hpp
#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QVector>
#include <QHash>
#include <QSet>

#include <QPixmap>

namespace smart_lt::api {

// This is a lightweight DTO for rendering marketplace cards in the UI.
struct ResourceItem {
    QString guid;
    QString slug;
    QString url;
    QString title;
    QString shortDescription;
    QString typeLabel;

    // Optional extras (may be empty)
    QString downloadUrl;
    QString iconPublicUrl;
    QString coverPublicUrl;
};

class ApiClient final : public QObject {
    Q_OBJECT
public:
    static ApiClient &instance();

    // Initializes the client: loads disk cache (if any) and optionally refreshes.
    // Safe to call multiple times.
    void init();

    // Triggers an async refresh (will obey cache TTL unless force=true).
    void fetchLowerThirds(bool force = false);

    // Async image fetch with disk+memory cache. Emits imageReady(url, pixmap).
    // Safe to call frequently; it will dedupe in-flight requests.
    void requestImage(const QString &imageUrl, int targetPx = 48);

    QVector<ResourceItem> lowerThirds() const;
    QString lastError() const;

signals:
    void lowerThirdsUpdated();
    void lowerThirdsFailed(const QString &err);

    void imageReady(const QString &imageUrl, const QPixmap &pixmap);
    void imageFailed(const QString &imageUrl, const QString &err);

private:
    explicit ApiClient(QObject *parent = nullptr);

    void loadCacheFromDisk();
    void saveCacheToDisk(const QByteArray &rawJson, qint64 fetchedAtEpochSec);
    bool isCacheFresh(qint64 nowEpochSec) const;
    QString cacheFilePath() const;

    void parseAndSet(const QByteArray &rawJson);
    QUrl buildLowerThirdsUrl() const;

    QString imageCacheDir() const;
    QString imageCachePathForUrl(const QString &imageUrl) const;

private:
    QVector<ResourceItem> m_lowerThirds;
    QString m_lastError;
    QByteArray m_lastRaw;
    qint64 m_cacheFetchedAt = 0; // epoch seconds
    bool m_inited = false;
    bool m_fetchInFlight = false;

    // image caches
    QHash<QString, QPixmap> m_pixCache;         // url -> pixmap
    QSet<QString> m_imgInFlight;                // url
};

} // namespace smart_lt::api
