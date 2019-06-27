#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <QDialog>
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

private:
    Ui::FileTransferDialog ftd;
    iFoxtrotSession *session;
};

#endif // FILETRANSFER_H
