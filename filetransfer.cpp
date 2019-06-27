#include <QDebug>
#include <QStringList>
#include <QTextCodec>

#include "filetransfer.h"
#include "ifoxtrotsession.h"

FileTransfer::FileTransfer(iFoxtrotSession *session, QWidget *parent) :
    QDialog(parent), session(session)
{
	ui.setupUi(this);
	ui.treeView->setModel(&model);
	session->receiveFile("//", [this](const QByteArray &data) -> void {
		QTextCodec *codec = QTextCodec::codecForName("Windows 1250");
		QStringList entries = codec->toUnicode(data.data()).split('\n');
		if (entries.last().isEmpty())
			entries.removeLast();
		qDebug() << entries;
		model.setStringList(entries);
	});
}

void FileTransfer::on_butDownload_clicked()
{
    qDebug() << __func__;
}
