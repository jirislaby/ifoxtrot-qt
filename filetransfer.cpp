#include <QDebug>

#include "filetransfer.h"
#include "ifoxtrotsession.h"

FileTransfer::FileTransfer(iFoxtrotSession *session, QWidget *parent) :
    QDialog(parent), session(session)
{
    ftd.setupUi(this);
    session->receiveFile("//", [this](const QByteArray &file) -> void {
        qDebug() << file;
    });
}

void FileTransfer::on_butDownload_clicked()
{
    qDebug() << __func__;
}
