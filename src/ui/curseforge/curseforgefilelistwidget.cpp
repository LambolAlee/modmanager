#include "curseforgefilelistwidget.h"
#include "ui_curseforgefilelistwidget.h"

#include <QStandardItemModel>

#include "ui/downloadpathselectmenu.h"
#include "curseforgemodbrowser.h"
#include "curseforgefileitemwidget.h"
#include "curseforge/curseforgemod.h"
#include "local/localmodpath.h"
#include "util/datetimesortitem.h"
#include "util/smoothscrollbar.h"
#include "util/funcutil.h"

CurseforgeFileListWidget::CurseforgeFileListWidget(CurseforgeModBrowser *parent) :
    QWidget(parent),
    ui(new Ui::CurseforgeFileListWidget)
{
    ui->setupUi(this);
    ui->fileListView->setModel(&model_);
    ui->fileListView->setVerticalScrollBar(new SmoothScrollBar(this));
    ui->fileListView->setProperty("class", "ModList");
    connect(ui->fileListView->verticalScrollBar(), &QAbstractSlider::valueChanged,  this , &CurseforgeFileListWidget::updateIndexWidget);
    connect(ui->fileListView->verticalScrollBar(), &QSlider::valueChanged, this, &CurseforgeFileListWidget::onListSliderChanged);
    ui->downloadPathSelect->hide();
    downloadPathSelectMenu_ = parent->downloadPathSelectMenu();
}

CurseforgeFileListWidget::CurseforgeFileListWidget(QWidget *parent, LocalMod *localMod) :
    QWidget(parent),
    ui(new Ui::CurseforgeFileListWidget)
{
    ui->setupUi(this);
    ui->fileListView->setModel(&model_);
    ui->fileListView->setVerticalScrollBar(new SmoothScrollBar(this));
    ui->fileListView->setProperty("class", "ModList");
    connect(ui->fileListView->verticalScrollBar(), &QAbstractSlider::valueChanged, this, &CurseforgeFileListWidget::updateIndexWidget);
    connect(ui->fileListView->verticalScrollBar(), &QSlider::valueChanged, this, &CurseforgeFileListWidget::onListSliderChanged);
    ui->downloadPathSelect->hide();
    downloadPathSelectMenu_ = new DownloadPathSelectMenu(this);
    ui->downloadPathSelect->setDefaultAction(downloadPathSelectMenu_->menuAction());
    ui->downloadPathSelect->setPopupMode(QToolButton::InstantPopup);
    setLocalMod(localMod);
}

CurseforgeFileListWidget::~CurseforgeFileListWidget()
{
    delete ui;
}

void CurseforgeFileListWidget::setMod(CurseforgeMod *mod)
{
    mod_ = mod;
    emit modChanged();
    ui->fileListView->setVisible(mod_);
    if(!mod_) return;
    connect(this, &CurseforgeFileListWidget::modChanged, disconnecter(
                connect(mod_, &CurseforgeMod::moreFileListReady, this, &CurseforgeFileListWidget::updateFileList),
                connect(mod_, &QObject::destroyed, this, [=]{ setMod(nullptr); })));

    updateFileList();
    if(!mod_->modInfo().fileCompleted()){
        ui->fileListView->setCursor(Qt::BusyCursor);
        mod_->acquireMoreFileList();
    }
}

void CurseforgeFileListWidget::updateUi()
{
    for(auto &&widget : findChildren<CurseforgeFileItemWidget *>())
        widget->updateUi();
}

void CurseforgeFileListWidget::updateFileList()
{
    model_.clear();
    for(int i = 0; i < mod_->modInfo().allFileList().size(); i++){
        auto item = new QStandardItem;
        model_.appendRow(item);
        auto &&fileInfo = mod_->modInfo().allFileList().at(i);
        item->setData(fileInfo.fileDate(), Qt::UserRole);
        item->setData(i, Qt::UserRole + 1);
        item->setSizeHint(QSize(0, 100));
    }
    model_.setSortRole(Qt::UserRole);
    model_.sort(0, Qt::DescendingOrder);
    ui->fileListView->setCursor(Qt::ArrowCursor);
}

void CurseforgeFileListWidget::paintEvent(QPaintEvent *event)
{
    updateIndexWidget();
    QWidget::paintEvent(event);
}

void CurseforgeFileListWidget::setDownloadPathSelectMenu(DownloadPathSelectMenu *newDownloadPathSelectMenu)
{
    downloadPathSelectMenu_ = newDownloadPathSelectMenu;
}

void CurseforgeFileListWidget::setLocalMod(LocalMod *newLocalMod)
{
    localMod_ = newLocalMod;
    if(localMod_){
        downloadPathSelectMenu_->setDownloadPath(localMod_->path());
        ui->downloadPathSelect->show();
    }
}

DownloadPathSelectMenu *CurseforgeFileListWidget::downloadPathSelectMenu() const
{
    return downloadPathSelectMenu_;
}

void CurseforgeFileListWidget::updateIndexWidget()
{
    auto beginRow = ui->fileListView->indexAt(QPoint(0, 0)).row();
    if(beginRow < 0) return;
    auto endRow = ui->fileListView->indexAt(QPoint(0, ui->fileListView->height())).row();
    if(endRow < 0)
        endRow = model_.rowCount() - 1;
    else
        //extra 2
        endRow += 2;
    for(int row = beginRow; row <= endRow && row < model_.rowCount(); row++){
        auto index = model_.index(row, 0);
        if(ui->fileListView->indexWidget(index)) continue;
        auto item = model_.item(row);
        auto &&fileInfo = mod_->modInfo().allFileList().at(item->data(Qt::UserRole + 1).toInt());
        auto itemWidget = new CurseforgeFileItemWidget(this, mod_, fileInfo);
        ui->fileListView->setIndexWidget(model_.indexFromItem(item), itemWidget);
        item->setSizeHint(QSize(0, itemWidget->height()));
    }
}

void CurseforgeFileListWidget::onListSliderChanged(int i)
{
    if(i >= ui->fileListView->verticalScrollBar()->maximum() - 1000){
        if(!mod_->modInfo().fileCompleted()){
            ui->fileListView->setCursor(Qt::BusyCursor);
            mod_->acquireMoreFileList();
        }
    }
}
