#include "modmanager.h"
#include "ui_modmanager.h"

#include "quazip.h"
#include "quazipfile.h"
#include "MurmurHash2.h"

#include <QDebug>
#include <QDir>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

#include "localmodbrowser.h"
#include "curseforgemodbrowser.h"
#include "modrinthmodbrowser.h"
#include "downloadbrowser.h"
#include "preferences.h"
#include "browsermanagerdialog.h"
#include "localmodbrowsersettingsdialog.h"
#include "gameversion.h"
#include "config.h"

ModManager::ModManager(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ModManager),
    updateVersionsWatcher_(new QFutureWatcher<void>(this)),
    downloadItem_(new QTreeWidgetItem({tr("Download")})),
    exploreItem_(new QTreeWidgetItem({tr("Explore")})),
    localItem_(new QTreeWidgetItem({tr("Local")}))
{
    ui->setupUi(this);
    ui->splitter->setStretchFactor(0, 1);
    ui->splitter->setStretchFactor(1, 2);

    //set tree widget
    for (const auto &item : {downloadItem_, exploreItem_, localItem_}){
        item->setForeground(0, Qt::gray);
        ui->browserTreeWidget->addTopLevelItem(item);
    }
    ui->browserTreeWidget->expandAll();

    //load mod dirs in config
    Config config;
    for(const auto &variant : config.getDirList())
        modDirList_ << ModDirInfo::fromVariant(variant);

    //Downloader
    auto downloadBrowser = new DownloadBrowser(this);
    auto downloaderItem = new QTreeWidgetItem(downloadItem_, {tr("Downloader")});
    downloadItem_->addChild(downloaderItem);
    downloaderItem->setIcon(0, QIcon::fromTheme("download"));
    ui->stackedWidget->addWidget(downloadBrowser);

    //Curseforge
    auto curseforgeModBrowser = new CurseforgeModBrowser(this);
    auto curseforgeItem = new QTreeWidgetItem(exploreItem_, {tr("Curseforge")});
    exploreItem_->addChild(curseforgeItem);
    curseforgeItem->setIcon(0, QIcon(":/image/curseforge.svg"));
    ui->stackedWidget->addWidget(curseforgeModBrowser);

    //Modrinth
    auto modrinthModBrowser = new ModrinthModBrowser(this);
    auto modrinthItem = new QTreeWidgetItem(exploreItem_, {tr("Modrinth")});
    exploreItem_->addChild(modrinthItem);
    modrinthItem->setIcon(0, QIcon(":/image/modrinth.svg"));
    ui->stackedWidget->addWidget(modrinthModBrowser);

    //Local
    for(const auto &modDirInfo : qAsConst(modDirList_)){
        if(modDirInfo.exists()) {
            auto item = new QTreeWidgetItem(localItem_, {modDirInfo.showText()});
            localItem_->addChild(item);
            item->setIcon(0, QIcon::fromTheme("folder"));
            auto localModBrowser = new LocalModBrowser(this, modDirInfo);
            localItemList_ << item;
            localModBrowserList_.append(localModBrowser);
            ui->stackedWidget->addWidget(localModBrowser);
        }
    }

    ui->browserTreeWidget->setCurrentItem(curseforgeItem);

    //check and update version
    QFuture<void> future = QtConcurrent::run(&GameVersion::initVersionList);
    updateVersionsWatcher_->setFuture(future);
    connect(updateVersionsWatcher_, &QFutureWatcher<void>::finished, curseforgeModBrowser, &CurseforgeModBrowser::updateVersions);
}

ModManager::~ModManager()
{
    delete ui;
}

void ModManager::refreshBrowsers()
{
    //load mod dirs in config
    Config config;
    auto oldCount = modDirList_.size();
    for(const auto &variant : config.getDirList()){
        auto modDirInfo = ModDirInfo::fromVariant(variant);
        if(!modDirInfo.exists()) continue;
        auto i = modDirList_.indexOf(modDirInfo);
        if(i < 0){
            //not present, new one
            modDirList_ << modDirInfo;
            auto item = new QTreeWidgetItem(localItem_, {modDirInfo.showText()});
            localItem_->addChild(item);
            item->setIcon(0, QIcon::fromTheme("folder"));
            auto localModBrowser = new LocalModBrowser(this, modDirInfo);
            localItemList_.append(item);
            localModBrowserList_.append(localModBrowser);
            ui->stackedWidget->addWidget(localModBrowser);
        } else{
            //present, move position
            oldCount--;
            auto j = i + 3;
            modDirList_ << modDirList_.takeAt(i);
            localItemList_ << localItemList_.takeAt(i);
            localModBrowserList_ << localModBrowserList_.takeAt(i);
            localItem_->addChild(localItem_->takeChild(i));
            auto widget = ui->stackedWidget->widget(j);
            ui->stackedWidget->removeWidget(widget);
            ui->stackedWidget->addWidget(widget);
        }
    }
    //remove remained mod dir
    auto i = oldCount;
    while (i--) {
        auto j = i + 3;
        modDirList_.removeAt(i);
        localItemList_.removeAt(i);
        localModBrowserList_.at(i);
        delete localItem_->takeChild(i);
        auto widget = ui->stackedWidget->widget(j);
        ui->stackedWidget->removeWidget(widget);
        //TODO: remove it improperly will cause program to crash
//        widget->deleteLater();
    }

}

void ModManager::on_actionPreferences_triggered()
{
    auto preferences = new Preferences(this);
    preferences->show();
}


void ModManager::on_actionManage_Browser_triggered()
{
    auto dialog = new BrowserManagerDialog(this);
    connect(dialog, &BrowserManagerDialog::accepted, this, &ModManager::refreshBrowsers);
    dialog->show();
}


void ModManager::on_browserTreeWidget_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    auto parent = current->parent();
    if(parent == nullptr){
        if(previous != nullptr)
            ui->browserTreeWidget->setCurrentItem(previous);
        return;
    } else if(parent == downloadItem_){
        ui->stackedWidget->setCurrentIndex(0);
    } else if(parent == exploreItem_){
        auto index = parent->indexOfChild(current);
        ui->stackedWidget->setCurrentIndex(1 + index);
    } else if(parent == localItem_){
        auto index = parent->indexOfChild(current);
        ui->stackedWidget->setCurrentIndex(3 + index);
    }
}


void ModManager::on_browserTreeWidget_itemDoubleClicked(QTreeWidgetItem *item, int /*column*/)
{
    if(item->parent() != localItem_) return;
    auto index = localItem_->indexOfChild(item);
    auto modDirInfo = modDirList_.at(index);
    auto dialog = new LocalModBrowserSettingsDialog(this, modDirInfo);
    connect(dialog, &LocalModBrowserSettingsDialog::settingsUpdated, this, [=](const ModDirInfo &newInfo){
        modDirList_[index] = newInfo;
        localItemList_[index]->setText(0, newInfo.showText());
        localModBrowserList_[index]->setModDirInfo(newInfo);
    });
    connect(updateVersionsWatcher_, &QFutureWatcher<void>::finished, dialog, &LocalModBrowserSettingsDialog::updateVersions);
    dialog->exec();
}

