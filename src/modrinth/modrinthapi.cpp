#include "modrinthapi.h"

#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QDebug>
#include <QJsonArray>

#include "util/tutil.hpp"
#include "util/mmlogger.h"
#include "config.hpp"

const QString ModrinthAPI::PREFIX = "https://api.modrinth.com";

ModrinthAPI::ModrinthAPI(QObject *parent) :
    QObject(parent)
{
    accessManager_.setTransferTimeout(Config().getNetworkRequestTimeout());
}

ModrinthAPI *ModrinthAPI::api()
{
    static ModrinthAPI api;
    return &api;
}

Reply<QList<ModrinthModInfo>> ModrinthAPI::searchMods(const QString name, int index, const QList<GameVersion> &versions, ModLoaderType::Type type, const QList<QString> &categories, int sort)
{
    QUrl url = PREFIX + "/v2/search";

    //url query
    QUrlQuery urlQuery;

    //name
    urlQuery.addQueryItem("query", name);
    //sort
    QString str;
    switch (sort) {
    case 0:
        str = "relevance";
        break;
    case 1:
        str = "downloads";
        break;
    case 2:
        str = "follows";//New add
        break;
    case 3:
        str = "updated";
        break;
    case 4:
        str = "newest";
        break;
    }
    urlQuery.addQueryItem("index", str);
    //index - offset
    urlQuery.addQueryItem("offset", QString::number(index));
    //search page size
    urlQuery.addQueryItem("limit", QString::number(Config().getSearchResultCount()));

    QJsonArray facets;
    //game version
    QJsonArray versionFacets;
    for(const auto &version : versions)
        if(version != GameVersion::Any)
            versionFacets << "versions:" + version;
    if(!versionFacets.isEmpty())
        facets << versionFacets;

    //loader type
    QJsonArray categoryFacets;
    if(type != ModLoaderType::Any)
        categoryFacets << "categories:" + ModLoaderType::toString(type);

    //loader type
    for(const auto &category : categories)
        if(!category.isEmpty())
            categoryFacets << "categories:" + category;

    if(!categoryFacets.isEmpty())
        facets << categoryFacets;

    if(!facets.isEmpty())
        urlQuery.addQueryItem("facets", QJsonDocument(facets).toJson(QJsonDocument::Compact));

    url.setQuery(urlQuery);
    QNetworkRequest request(url);
    MMLogger::network(this) << url;
    auto reply = accessManager_.get(request);
    return { reply,  [=]{
            //parse json
            QJsonParseError error;
            QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &error);
            if(error.error != QJsonParseError::NoError) {
                qDebug("%s", error.errorString().toUtf8().constData());
                return QList<ModrinthModInfo>{};
            }

            auto resultList = value(jsonDocument.toVariant(), "hits").toList();
            QList<ModrinthModInfo> modInfoList;
            for(const auto &result : qAsConst(resultList))
                modInfoList << ModrinthModInfo::fromSearchVariant(result);

            return modInfoList;
        } };
}

Reply<ModrinthModInfo> ModrinthAPI::getInfo(const QString &id)
{
    //id: "local-xxxxx" ???
    auto modId = id.startsWith("local-")? id.right(id.size() - 6) : id;
    QUrl url = PREFIX + "/v2/project/" + modId;
    QNetworkRequest request(url);
    MMLogger::network(this) << url;
    auto reply = accessManager_.get(request);
    return { reply,  [=]{
            //parse json
            QJsonParseError error;
            QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                qDebug("%s", error.errorString().toUtf8().constData());
                return ModrinthModInfo();
            }

            auto result = jsonDocument.toVariant();
            auto modrinthModInfo = ModrinthModInfo::fromVariant(result);

            return modrinthModInfo;
        } };
}

Reply<QList<ModrinthFileInfo> > ModrinthAPI::getVersions(const QString &id)
{
    //id: "local-xxxxx" ???
    auto modId = id.startsWith("local-")? id.right(id.size() - 6) : id;
    QUrl url = PREFIX + "/v2/project/" + modId + "/version";

    QNetworkRequest request(url);
    MMLogger::network(this) << url;
    auto reply = accessManager_.get(request);
    return { reply, [=]{
            //parse json
            QJsonParseError error;
            QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                qDebug("%s", error.errorString().toUtf8().constData());
                return QList<ModrinthFileInfo>{};
            }

            auto resultList = jsonDocument.toVariant().toList();
            QList<ModrinthFileInfo> fileInfoList;
            for(const auto &result : qAsConst(resultList))
                fileInfoList << ModrinthFileInfo::fromVariant(result);
            return fileInfoList;
        } };
}

Reply<ModrinthFileInfo> ModrinthAPI::getVersion(const QString &version)
{
    QUrl url = PREFIX + "/v2/version/" + version;
    QNetworkRequest request(url);
    MMLogger::network(this) << url;
    auto reply = accessManager_.get(request);
    return { reply, [=]{
            //parse json
            QJsonParseError error;
            QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                qDebug("%s", error.errorString().toUtf8().constData());
                return ModrinthFileInfo();
            }

            auto result = jsonDocument.toVariant();
            return ModrinthFileInfo::fromVariant(result);
        } };
}

Reply<QString> ModrinthAPI::getAuthor(const QString &authorId)
{
    QUrl url = PREFIX + "/v2/user/" + authorId;
    QNetworkRequest request(url);
    MMLogger::network(this) << url;
    auto reply = accessManager_.get(request);
    return { reply, [=]{
            //parse json
            QJsonParseError error;
            QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                qDebug("%s", error.errorString().toUtf8().constData());
                return QString();
            }

            auto result = jsonDocument.toVariant();
            return value(result, "name").toString();
        } };
}

Reply<ModrinthFileInfo> ModrinthAPI::getVersionFileBySha1(const QString sha1)
{
    QUrl url = PREFIX + "/v2/version_file/" + sha1;

    //url query
    QUrlQuery urlQuery;
    //set type sha1
    urlQuery.addQueryItem("algorithm", "sha1");

    url.setQuery(urlQuery);
    QNetworkRequest request(url);
    MMLogger::network(this) << url;
    auto reply = accessManager_.get(request);
    return { reply, [=]{
            //parse json
            QJsonParseError error;
            QJsonDocument jsonDocument = QJsonDocument::fromJson(reply->readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                qDebug("%s", error.errorString().toUtf8().constData());
                return ModrinthFileInfo();
            }
            auto result = jsonDocument.toVariant();
            return ModrinthFileInfo::fromVariant(result);
        } };
}

const QList<std::tuple<QString, QString> > &ModrinthAPI::getCategories()
{
    static const QList<std::tuple<QString, QString>> categories{
        { tr("World generation"), "worldgen" },
        { tr("Technology"), "technology" },
        { tr("Food"), "food" },
        { tr("Magic"), "magic" },
        { tr("Storage"), "storage" },
        { tr("Library"), "library" },
        { tr("Adventure"), "adventure" },
        { tr("Utility"), "utility" },
        { tr("Decoration"), "decoration" },
        { tr("Miscellaneous"), "misc" },
        { tr("Optimization"), "optimization" },
        { tr("Equipment"), "equipment" },
        { tr("Cursed"), "cursed" }
    };
    return categories;
}
