#include "modlinkcache.h"

#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "localmod.h"
#include "curseforge/curseforgemod.h"
#include "modrinth/modrinthmod.h"
#include "util/tutil.hpp"

ModLinkCache::ModLinkCache(const QString &path) :
    path_(path)
{
    QDir dir(path_);
    QFile file(dir.absoluteFilePath("link_cache.json"));
    if(!file.open(QIODevice::ReadOnly)) return;
    auto bytes = file.readAll();
    file.close();

    //parse json
    QJsonParseError error;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug("%s", error.errorString().toUtf8().constData());
        return;
    }
    QList list = jsonDocument.toVariant().toList();
    for(const auto &variant : list){
        auto link = ModLink::fromVariant(variant);
        linkCaches_.insert(link.id(), link);
    }
}

void ModLinkCache::addCache(LocalMod *localMod)
{
    ModLink link;
    link.id_ = localMod->commonInfo()->id();
    if(localMod->curseforgeMod()){
        link.curseforgeId_ = localMod->curseforgeMod()->modInfo().id();
        link.curseforgeFileId_ = localMod->curseforgeFileId();
    }
    if(localMod->modrinthMod()){
        link.modrinthId_ = localMod->modrinthMod()->modInfo().id();
        link.modrinthFileId_ = localMod->modrinthFileId();
    }

    linkCaches_.insert(link.id_, link);
}

void ModLinkCache::saveToFile()
{
    QJsonObject map;
    for(auto link : qAsConst(linkCaches_))
        map.insert(link.id(), link.toJsonObject());

    QDir dir(path_);
    QFile file(dir.absoluteFilePath("link_cache.json"));
    if(!file.open(QIODevice::WriteOnly)) return;

    QJsonDocument doc(map);
    file.write(doc.toJson());
}

QJsonObject ModLinkCache::ModLink::toJsonObject()
{
    QJsonObject object{
        {"id", id_}
    };

    if(curseforgeId_)
        object.insert("curseforge", QJsonObject{
                          {"id", curseforgeId_},
                          {"fileId", curseforgeFileId_}
                      });
    if(!modrinthId_.isEmpty())
        object.insert("modrinth", QJsonObject{
                          {"id", modrinthId_},
                          {"fileId", modrinthFileId_}
                      });
    return object;
}

ModLinkCache::ModLink ModLinkCache::ModLink::fromVariant(const QVariant &variant)
{
    ModLink link;
    link.id_ = value(variant, "id").toString();
    link.curseforgeId_ = value(variant, "curseforge", "id").toInt();
    link.curseforgeFileId_ = value(variant, "curseforge", "fileId").toInt();
    link.modrinthId_ = value(variant, "modrinth", "id").toString();
    link.modrinthFileId_ = value(variant, "modrinth", "fileId").toString();
    return link;
}

const QString &ModLinkCache::ModLink::id() const
{
    return id_;
}

int ModLinkCache::ModLink::curseforgeId() const
{
    return curseforgeId_;
}

int ModLinkCache::ModLink::curseforgeFileId() const
{
    return curseforgeFileId_;
}

const QString &ModLinkCache::ModLink::modrinthId() const
{
    return modrinthId_;
}

const QString &ModLinkCache::ModLink::modrinthFileId() const
{
    return modrinthFileId_;
}
