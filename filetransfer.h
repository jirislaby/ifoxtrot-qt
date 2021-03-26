#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <functional>

#include <QDialog>
#include <QStringListModel>
#include <QWidget>

#include "treemodel.h"
#include "ui_filetransfer.h"

class iFoxtrotSession;

class FileTransfer : public QDialog
{
    Q_OBJECT

public:
    FileTransfer(iFoxtrotSession *session, QWidget *parent = nullptr);

    void load(const QString &path,
	      const std::function<void(const QStringList &)> &callback);

private slots:
    void on_butDownload_clicked();

    void on_butReload_clicked();

    void on_treeView_doubleClicked(const QModelIndex &index);

private:
    Ui::FileTransferDialog ui;
    iFoxtrotSession *session;
    TreeModel model;

    void load();
};

#endif // FILETRANSFER_H
