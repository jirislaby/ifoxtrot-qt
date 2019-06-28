#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <QDialog>
#include <QStringListModel>
#include <QWidget>

#include "ui_filetransfer.h"

class iFoxtrotSession;

class FileTransfer : public QDialog
{
    Q_OBJECT

public:
    FileTransfer(iFoxtrotSession *session, QWidget *parent = nullptr);

private slots:
    void on_butDownload_clicked();

    void on_butReload_clicked();

    void on_treeView_doubleClicked(const QModelIndex &index);

private:
    Ui::FileTransferDialog ui;
    iFoxtrotSession *session;
    QStringListModel model;

    void loadByPath();
};

#endif // FILETRANSFER_H
