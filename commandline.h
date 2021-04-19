#ifndef COMMANDLINE_H
#define COMMANDLINE_H

#include "ifoxtrotsession.h"

#include <QApplication>
#include <QAtomicInt>
#include <QCommandLineParser>
#include <QObject>
#include <QList>
#include <QPair>
#include <QString>
#include <QStringList>

class CommandLine : public QObject
{
	Q_OBJECT
public:
	typedef QPair<QString, QString> SetPair;

	static void handleTUI(QApplication &a, QCommandLineParser &parser);

	CommandLine(bool quiet, QObject *parent = nullptr);
	~CommandLine();

	void setConnectionDetails(QCommandLineParser &parser);

	void run();

	const iFoxtrotSession *getSession() const { return &session; }

	void addGets(const QStringList &gets) { this->gets.append(gets); }
	void addSet(const SetPair &set) { sets.append(set); }

private slots:
	void runSlot();
	void connected();
	void conStatusUpdate(const QString &status);
	void disconnected();
	void sockError(QAbstractSocket::SocketError socketError);
	void conError(const QString &reason);

signals:
	void finished();

private:
	void connectToHost();
	void deref(QAtomicInt *aint);

	iFoxtrotSession session;

	bool quiet;
	QString host;
	quint16 port;
	QString PLC;

	QStringList gets;
	QList<SetPair> sets;
};

#endif // COMMANDLINE_H
