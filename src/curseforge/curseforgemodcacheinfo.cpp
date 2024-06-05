#include "curseforgemodcacheinfo.h"

#include <QDir>
#include <QJsonObject>
#include <QStandardPaths>

#include "download/assetcache.h"
#include "util/tutil.hpp"

CurseforgeModCacheInfo::CurseforgeModCacheInfo(int addonId) :
    id_(addonId)
{}

CurseforgeModCacheInfo CurseforgeModCacheInfo::fromVariant(const QVariant &variant)
{
    CurseforgeModCacheInfo info;
    info.id_ = value(variant, "id").toInt();
    info.name_ = value(variant, "name").toString();
    info.summary_ = value(variant, "summary").toString();
    if(variant.toMap().contains("slug"))
        info.slug_ = value(variant, "slug").toString();
    info.iconUrl_ = value(variant, "iconUrl").toUrl();
    info.gamePopularityRank_ = value(variant, "gamePopularityRank").toDouble();
    return info;
}

QJsonObject CurseforgeModCacheInfo::toJsonObject() const
{
    QJsonObject object;
    object.insert("id", id_);
    object.insert("name", name_);
    object.insert("summary", summary_);
    object.insert("slug", slug_);
    object.insert("iconUrl", iconUrl_.toString());
    object.insert("gamePopularityRank", gamePopularityRank_);
    return object;
}

bool CurseforgeModCacheInfo::operator==(const CurseforgeModCacheInfo &other) const
{
    return toJsonObject() == other.toJsonObject();
}

bool CurseforgeModCacheInfo::loadIcon()
{
    AssetCache iconCache(nullptr, iconUrl_, iconUrl_.fileName(), cachePath());
    if(iconCache.exists()){
        icon_.load(iconCache.destFilePath());
        return true;
    } else
        return false;
}

const QString &CurseforgeModCacheInfo::cachePath()
{
    static const QString path =
            QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
            .absoluteFilePath("curseforge/mods");
    return path;
}

int CurseforgeModCacheInfo::id() const
{
    return id_;
}

const QString &CurseforgeModCacheInfo::name() const
{
    return name_;
}

const QString &CurseforgeModCacheInfo::summary() const
{
    return summary_;
}

const QString &CurseforgeModCacheInfo::slug() const
{
    return slug_;
}

const QUrl &CurseforgeModCacheInfo::iconUrl() const
{
    return iconUrl_;
}

const QPixmap &CurseforgeModCacheInfo::icon() const
{
    return icon_;
}

double CurseforgeModCacheInfo::gamePopularityRank() const
{
    return gamePopularityRank_;
}
