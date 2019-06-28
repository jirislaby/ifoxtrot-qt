#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QStringList>
#include <QTextCodec>

#include "filetransfer.h"
#include "ifoxtrotsession.h"

FileTransfer::FileTransfer(iFoxtrotSession *session, QWidget *parent) :
    QDialog(parent), session(session)
{
	ui.setupUi(this);
	ui.treeView->setModel(&model);
	loadByPath();
}

void FileTransfer::loadByPath()
{
	const QString path = ui.lineEditPath->text();

	ui.groupRemote->setEnabled(false);

	session->receiveFile(path, [this](const QByteArray &data) -> void {
		QTextCodec *codec = QTextCodec::codecForName("Windows 1250");
		QStringList entries = codec->toUnicode(data.data()).split('\n');
		if (entries.count() && entries.last().isEmpty())
			entries.removeLast();
		qDebug() << entries;
		model.setStringList(entries);
		ui.groupRemote->setEnabled(true);
	});
}

void FileTransfer::on_butDownload_clicked()
{
	const QModelIndex index = ui.treeView->selectionModel()->currentIndex();
	if (!index.isValid())
		return;
	QString src = model.data(index).toString();
	if (src.endsWith('/'))
		return;

	QString suggestedName = src.mid(src.lastIndexOf('/') + 1);
	QString destName = QFileDialog::getSaveFileName(this, tr("Save File"),
			suggestedName);
	if (destName.isEmpty())
		return;

	auto dest = new QFile(destName);

	if (!dest->open(QIODevice::WriteOnly)) {
		qWarning() << "cannot open file for writing" <<
		              dest->fileName();
		return;
	}

	session->receiveFile(src, [this, dest](const QByteArray &data) -> void {
		dest->write(data);
		dest->close();
		delete dest;
		ui.groupRemote->setEnabled(true);
	});
}

void FileTransfer::on_butReload_clicked()
{
	loadByPath();
}

void FileTransfer::on_treeView_doubleClicked(const QModelIndex &index)
{
	QString str = model.data(index).toString();

	qDebug() << __func__ << str;

	if (str.endsWith('/')) {
		ui.lineEditPath->setText(str);
		loadByPath();
		return;
	}

	ui.groupRemote->setEnabled(false);

	session->receiveFile(str, [this](const QByteArray &data) -> void {
		QTextCodec *codec = QTextCodec::codecForName("Windows 1250");
		QString strData = codec->toUnicode(data.data());
		ui.plainTextEdit->setPlainText(strData);
		ui.groupRemote->setEnabled(true);
	});
}
