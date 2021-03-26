#include <functional>

#include <QDebug>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QStringList>
#include <QTextCodec>

#include "treeitem.h"
#include "filetransfer.h"
#include "ifoxtrotsession.h"

FileTransfer::FileTransfer(iFoxtrotSession *session, QWidget *parent) :
    QDialog(parent), session(session), model(this)
{
	ui.setupUi(this);
	ui.treeView->setModel(&model);
	load();
}

void FileTransfer::load()
{
	load("//", [this](const QStringList &entries) -> void {
		auto root = new TreeItem("//");
		for (auto e : entries)
			root->addChild(e.mid(2));
		model.setRoot(root);
	});
}

void FileTransfer::load(const QString &path,
			const std::function<void(const QStringList &)> &callback)
{
	ui.groupRemote->setEnabled(false);

	session->receiveFile(path, [this, callback](const QByteArray &data) -> void {
		QTextCodec *codec = QTextCodec::codecForName("Windows 1250");
		QStringList entries = codec->toUnicode(data.data()).split('\n');
		if (entries.count() && entries.last().isEmpty())
			entries.removeLast();
		qDebug() << entries;
		callback(entries);
		ui.groupRemote->setEnabled(true);
	});
}

void FileTransfer::on_butDownload_clicked()
{
	auto index = ui.treeView->selectionModel()->currentIndex();
	if (!index.isValid() || model.hasChildren(index))
		return;

	QString src = model.getPath(index);
	QString destName = QFileDialog::getSaveFileName(this, tr("Save File"),
			model.getName(index));
	if (destName.isEmpty())
		return;

	auto dest = new QFile(destName);

	if (!dest->open(QIODevice::WriteOnly)) {
		QMessageBox mbox;
		mbox.setIcon(QMessageBox::Critical);
		mbox.setWindowTitle(tr("Error"));
		mbox.setText(tr("<b>Cannot open file '").
			     append(dest->fileName()).
			     append(tr("' for writing!</b><br/><br/>")).
			     append(dest->errorString()));
		mbox.exec();
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
	load();
}

void FileTransfer::on_treeView_doubleClicked(const QModelIndex &index)
{
	if (model.hasChildren(index))
		return;

	ui.groupRemote->setEnabled(false);

	session->receiveFile(model.getPath(index), [this](const QByteArray &data) -> void {
		QTextCodec *codec = QTextCodec::codecForName("Windows 1250");
		QString strData = codec->toUnicode(data.data());
		ui.plainTextEdit->setPlainText(strData);
		ui.groupRemote->setEnabled(true);
	});
}
