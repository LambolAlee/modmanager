#ifndef CURSEFORGEMOD_H
#define CURSEFORGEMOD_H

#include <QObject>

#include <network/reply.hpp>

#include "curseforgemodinfo.h"
#include "tag/tagable.h"

class LocalModPath;
class QAria2Downloader;
class LocalMod;
class CurseforgeAPI;

class CurseforgeMod : public QObject, public Tagable
{
    Q_OBJECT
public:
    CurseforgeMod(QObject *parent, int id);
    CurseforgeMod(LocalMod *parent, int id);
    CurseforgeMod(QObject *parent, const CurseforgeModInfo &info);
    CurseforgeMod(LocalMod *parent, const CurseforgeModInfo &info);
    ~CurseforgeMod();

    void acquireBasicInfo();
    void acquireIcon();
    void acquireDescription();
    std::shared_ptr<Reply<QList<CurseforgeFileInfo>, int>> acquireMoreFileList(GameVersion version = GameVersion::Any, bool clear = false);

    const CurseforgeModInfo &modInfo() const;
    void download(const CurseforgeFileInfo &fileInfo, LocalModPath *downloadPath = nullptr);
    QAria2Downloader *downloader() const;

signals:
    void tagsChanged() override;

    void basicInfoReady();
    void iconReady();
    void descriptionReady();
    void moreFileListReady(QList<CurseforgeFileInfo> fileInfos);

    void downloadStarted();

private:
    CurseforgeAPI *api_;
    CurseforgeModInfo modInfo_;
    QAria2Downloader *downloader_;

    std::shared_ptr<Reply<CurseforgeModInfo>> basicInfoGetter_;
    bool gettingIcon_ = false;
    std::shared_ptr<Reply<QString>> descriptionGetter_;
    std::shared_ptr<Reply<QList<CurseforgeFileInfo>, int>> allFileListGetter_;
};

#endif // CURSEFORGEMOD_H
