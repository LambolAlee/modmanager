#include "githubfilelistwidget.h"
#include "githubrepobrowser.h"
#include "ui_githubfilelistwidget.h"

#include <QStandardItem>

#include "local/localmodpath.h"
#include "githubfileitemwidget.h"
#include "github/githubrelease.h"
#include "util/smoothscrollbar.h"
#include "util/funcutil.h"

GitHubFileListWidget::GitHubFileListWidget(GitHubRepoBrowser *parent) :
    GitHubFileListWidget(static_cast<QWidget *>(parent))
{
    browser_ = parent;
}

GitHubFileListWidget::GitHubFileListWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GitHubFileListWidget),
    model_(new QStandardItemModel(this))
{
    ui->setupUi(this);
    ui->fileListView->setModel(model_);
    ui->fileListView->setVerticalScrollBar(new SmoothScrollBar(this));
    ui->fileListView->setProperty("class", "ModList");
    connect(ui->fileListView->verticalScrollBar(), &QAbstractSlider::valueChanged,  this , &GitHubFileListWidget::updateIndexWidget);
}

GitHubFileListWidget::~GitHubFileListWidget()
{
    delete ui;
}

void GitHubFileListWidget::setRelease(GitHubRelease *release)
{
    release_= release;
    emit releaseChanged();
    ui->fileListView->setVisible(release_);
    if(!release_) return;
    connect(this, &GitHubFileListWidget::releaseChanged, disconnecter(
                connect(release_, &QObject::destroyed, this, [=]{setRelease(nullptr); })));

    updateFileList();
}

void GitHubFileListWidget::updateFileList()
{
    model_->clear();
    for(int i = 0; i < release_->info().assets().size(); i++){
        auto item = new QStandardItem;
        model_->appendRow(item);
//        auto &&fileInfo = release_->info().assets().at(i);
//        item->setData(fileInfo.fileDate(), Qt::UserRole);
        item->setData(i, Qt::UserRole + 1);
        item->setSizeHint(QSize(0, 100));
    }
//    model_->setSortRole(Qt::UserRole);
//    model_->sort(0, Qt::DescendingOrder);
    ui->fileListView->setCursor(Qt::ArrowCursor);
    updateIndexWidget();
}

void GitHubFileListWidget::updateIndexWidget()
{
    auto beginRow = ui->fileListView->indexAt(QPoint(0, 0)).row();
    if(beginRow < 0) return;
    auto endRow = ui->fileListView->indexAt(QPoint(0, ui->fileListView->height())).row();
    if(endRow < 0)
        endRow = model_->rowCount() - 1;
    else
        //extra 2
        endRow += 2;
    for(int row = 0; row < model_->rowCount(); row++){
        auto index = model_->index(row, 0);
        if(row >= beginRow && row <= endRow){
            if(ui->fileListView->indexWidget(index)) continue;
            auto item = model_->item(row);
            auto &&fileInfo = release_->info().assets().at(item->data(Qt::UserRole + 1).toInt());
            auto itemWidget = new GitHubFileItemWidget(this, fileInfo);
            ui->fileListView->setIndexWidget(model_->indexFromItem(item), itemWidget);
            item->setSizeHint(QSize(0, itemWidget->height()));
        } else{
            if(auto widget = ui->fileListView->indexWidget(index)){
                ui->fileListView->setIndexWidget(index, nullptr);
                delete widget;
            }
        }
    }
}

void GitHubFileListWidget::paintEvent(QPaintEvent *event)
{
    updateIndexWidget();
    QWidget::paintEvent(event);
}

DownloadPathSelectMenu *GitHubFileListWidget::downloadPathSelectMenu() const
{
    return browser_? browser_->downloadPathSelectMenu() : downloadPathSelectMenu_;
}

void GitHubFileListWidget::setBrowser(GitHubRepoBrowser *newBrowser)
{
    browser_ = newBrowser;
}
