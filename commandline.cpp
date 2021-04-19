#include "commandline.h"
#include "ifoxtrotctl.h"

#include <QApplication>
#include <QAtomicInt>
#include <QCommandLineParser>
#include <QtGlobal>

void CommandLine::handleTUI(QApplication &a, QCommandLineParser &parser)
{
	auto gets = parser.values("g");
	auto sets = parser.values("s");

	if (!gets.size() && !sets.size())
		return;

	CommandLine cmdline(parser.isSet("q"), &a);

	QObject::connect(&cmdline, &CommandLine::finished,
			 &QCoreApplication::quit);

	cmdline.setConnectionDetails(parser);

	cmdline.addGets(gets);

	for (auto s : sets) {
		auto itemVal = s.split('=');
		if (itemVal.size() != 2) {
			qCritical().noquote() << tr("Invalid set command!");
			parser.showHelp(1);
		}
		cmdline.addSet(SetPair(itemVal[0], itemVal[1]));
	}

	cmdline.run();

	int ret = a.exec();
	cmdline.close();
	exit(ret);
}

CommandLine::CommandLine(bool quiet, QObject *parent) :
	QObject(parent), session(false), quiet(quiet)
{
	connect(&session, &iFoxtrotSession::connected, this, &CommandLine::connected);
	connect(&session, &iFoxtrotSession::conStatusUpdate, this, &CommandLine::conStatusUpdate);
	connect(&session, &iFoxtrotSession::disconnected, this, &CommandLine::disconnected);
	connect(&session, &iFoxtrotSession::sockError, this, &CommandLine::sockError);
	connect(&session, &iFoxtrotSession::conError, this, &CommandLine::conError);
}

void CommandLine::close()
{
	session.close();
}

CommandLine::~CommandLine()
{
	close();
}

void CommandLine::setConnectionDetails(QCommandLineParser &parser)
{
	host = parser.value("H");
	if (host == "") {
		qCritical().noquote() << tr("Host not specified!");
		parser.showHelp(1);
	}

	bool ok;
	port = parser.value("p").toUShort(&ok);
	if (!ok) {
		qCritical().noquote() << tr("Invalid port specified!");
		parser.showHelp(1);
	}

	PLC = parser.value("P");
}

void CommandLine::runSlot()
{
	connectToHost();
}

void CommandLine::run()
{
	QTimer::singleShot(0, this, &CommandLine::runSlot);
}

void CommandLine::connectToHost()
{
	session.connectToHost(host, port, PLC);
}

void CommandLine::deref(QAtomicInt *aint)
{
	if (!aint->deref()) {
		delete aint;
		emit finished();
	}
}

void CommandLine::connected()
{
	if (!quiet) {
		qInfo().noquote() << tr("Connected to") << session.getPeerName();
		qInfo().noquote() << tr("PLC version:") << session.getPLCVersion();
	}

	session.write("DI:\n");

	auto aint = new QAtomicInt(gets.size() + !!sets.size());
	for (auto g : gets) {
		QString get("GET:");
		get.append(g).append('\n');
		auto rcv = new iFoxtrotReceiverGET(&session, get.toUtf8(),
						   [](const QString &foxName,
						   const QString &foxType,
						   const QString &prop,
						   const QString &value) {
			Q_UNUSED(foxType);
			qInfo().noquote() << foxName + '.' + prop << "=" << value;
		}, false);
		connect(rcv, &iFoxtrotReceiver::done, [this, rcv, aint] {
			rcv->deleteLater();
			deref(aint);
		});
		session.enqueueRcv(rcv);
	}

	if (sets.size()) {
		connect(&session, &iFoxtrotSession::bytesWritten,
			[this, aint](quint64) {
			if (!session.bytesToWrite())
				deref(aint);
		});
	}

	for (auto s : sets) {
		QString set("SET:");
		set.append(s.first).append(',').append(s.second).append('\n');
		session.write(set.toUtf8());
	}
}

void CommandLine::conStatusUpdate(const QString &status)
{
	qInfo().noquote() << session.getPeerName() + ":" << status;
}

void CommandLine::disconnected()
{
	qInfo().noquote() << tr("Disconnected");
}

void CommandLine::conError(const QString &reason)
{
	qInfo().noquote() << tr("Connection error:") << reason;
	disconnected();
}

void CommandLine::sockError(QAbstractSocket::SocketError socketError)
{
	qWarning().noquote() << tr("Socket error:") << socketError;
	disconnected();
}
