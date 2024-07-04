#ifndef OPTIFINEMODBROWSER_H
#define OPTIFINEMODBROWSER_H

#include "ui/explorebrowser.h"

#include "network/reply.hpp"
#include "optifine/optifinemodinfo.h"

class OptifineAPI;
class BMCLAPI;
class LocalModPath;
class ExploreStatusBarWidget;
class QStatusBar;
class OptifineManager;

class OptifineManagerProxyModel;
namespace Ui {
class OptifineModBrowser;
}

class OptifineModBrowser : public ExploreBrowser
{
    Q_OBJECT

public:
    explicit OptifineModBrowser(QWidget *parent = nullptr);
    ~OptifineModBrowser();

    void load() override;

public slots:
    void refresh() override;
    void searchModByPathInfo(LocalModPath *path) override;
    void updateUi() override;

    ExploreBrowser *another(QWidget *parent = nullptr) override;

private slots:
    void on_actionGet_OptiFabric_triggered();
    void on_actionGet_OptiForge_triggered();
    void on_versionSelect_currentIndexChanged(int index);
    void on_searchText_textChanged(const QString &arg1);
    void on_showPreview_toggled(bool checked);

private:
    Ui::OptifineModBrowser *ui;
    OptifineManager *manager_;
    OptifineManagerProxyModel *proxyModel_;
    QList<GameVersion> gameVersions_;
    bool inited_ = false;

    QWidget *getListViewIndexWidget(const QModelIndex &index) override;
};

#endif // OPTIFINEMODBROWSER_H
