#include <QDebug>
#include <QTime>
#include <QTextCodec>

#include "ifoxtrotctl.h"
#include "ifoxtrotreceiver.h"
#include "ifoxtrotsession.h"

iFoxtrotReceiver::iFoxtrotReceiver(iFoxtrotSession *session,
                                   const QByteArray &prefix,
                                   const QByteArray &write) :
        QObject(session), session(session), prefix(prefix), write(write)
{
}

iFoxtrotReceiverLine::iFoxtrotReceiverLine(iFoxtrotSession *session,
                                           const QByteArray &prefix,
                                           const QByteArray &write) :
        iFoxtrotReceiver(session, prefix, write),
        multiline(false)
{
}

qint64 iFoxtrotReceiverLine::handleData(QByteArray &data, bool *keep)
{
	while (session->bytesAvailable()) {
		char c;
		if (!session->getChar(&c)) {
			qWarning() << "socket.getChar failed";
			continue;
		}

		if (c == '\r')
			continue;

		if (c == '\n') {
			if (multiline) {
				if (data == prefix) {
					emit done();
					return 0;
				}
				*keep = true;
			}
			QTextCodec *codec = QTextCodec::codecForName("Windows 1250");
			pushLine(codec->toUnicode(data.data()));
			if (!multiline)
				emit done();
			return 0;
		}

		data.append(c);
	}

	return 1;
}

iFoxtrotReceiverDIFF::iFoxtrotReceiverDIFF(iFoxtrotSession *session) :
        iFoxtrotReceiverLine(session, "DIFF:", "")
{
}

void iFoxtrotReceiverDIFF::pushLine(const QString &line)
{
	static QRegularExpression DIFFRE("^DIFF:(.+)\\.GTSAP1_([^_]+)_(.+),(.+)$");
	QRegularExpressionMatch match = DIFFRE.match(line);

	if (!match.hasMatch()) {
		qDebug() << __PRETTY_FUNCTION__ << " unexpected DIFF line" << line;
		return;
	}

	QString foxName = match.captured(1);
	//QString foxType = match.captured(2);
	QString prop = match.captured(3);
	QString value = match.captured(4);

	session->handleDIFF(foxName, prop, value);

	//qDebug() << __PRETTY_FUNCTION__ << "DIFF" << foxName << foxType << prop << value;
}

iFoxtrotReceiverGETINFO::iFoxtrotReceiverGETINFO(iFoxtrotSession *session) :
        iFoxtrotReceiverLine(session, "GETINFO:", "GETINFO:\n")
{
	multiline = true;
}

void iFoxtrotReceiverGETINFO::pushLine(const QString &line)
{
	if (line.startsWith("GETINFO:VERSION_PLC,"))
		PLCVersion = line.mid(sizeof("GETINFO:VERSION_PLC,") - 1);
}

iFoxtrotReceiverSETCONF::iFoxtrotReceiverSETCONF(iFoxtrotSession *session,
                                                 const QByteArray &conf) :
        iFoxtrotReceiverLine(session, "WARNING:", conf)
{
}

bool iFoxtrotReceiverSETCONF::handleError(QByteArray &data)
{
	if (!data.startsWith("10 "))
		qWarning() << __func__ << "unknown error" << data;

	session->conFailed(tr("Cannot write ") + getWrite());

	return false;
}

void iFoxtrotReceiverSETCONF::pushLine(const QString &line)
{
	if (line.startsWith("WARNING:250 ") || line.startsWith("WARNING:251 "))
		return;
	qWarning() << __func__ << "unhandled " << line;
}

iFoxtrotReceiverGET::iFoxtrotReceiverGET(iFoxtrotSession *session,
                                         const QByteArray &write,
					 const CallbackFn &callbackFn,
					 bool multiline):
	iFoxtrotReceiverLine(session, "GET:", write), callbackFn(callbackFn)
{
	this->multiline = multiline;
}

bool iFoxtrotReceiverGET::handleError(QByteArray &data)
{
	if (data.startsWith("33 ")) {
		auto col = data.indexOf(':') + 2;

		qWarning().noquote() << __func__ << "bad register:" <<
					data.mid(col);

		return true;
	}

	qWarning() << __func__ << "unknown error" << data;

	return true;
}

void iFoxtrotReceiverGET::pushLine(const QString &line)
{
	static const QSet<QString> skip {
		"__PF_CRC",
		"__PLC_RUN"
	};
	static QRegularExpression GETRE("^GET:(.+)\\.GTSAP1_([^_]+)_(.+),(.+)$");

	QString crop = line.mid(4);
	crop.truncate(crop.indexOf(','));
	if (skip.contains(crop))
		return;

	QRegularExpressionMatch match = GETRE.match(line);
	if (!match.hasMatch()) {
		qDebug() << "wrong GET line" << line;
		return;
	}

	QString foxName = match.captured(1);
	QString foxType = match.captured(2);
	QString prop = match.captured(3);
	QString value = match.captured(4);

	callbackFn(foxName, foxType, prop, value);
}

iFoxtrotReceiverFile::iFoxtrotReceiverFile(iFoxtrotSession *session,
                                           const QByteArray &write,
                                           const QString &file,
                                           const CallbackFn &fun) :
        iFoxtrotReceiver(session, "GETFILE:", write), file(file),
        callbackFn(fun), continuation(0)
{
}

bool iFoxtrotReceiverFile::handleError(QByteArray &data)
{
	if (!data.startsWith("41 "))
		qDebug() << __PRETTY_FUNCTION__ << data;
	return false;
}

qint64 iFoxtrotReceiverFile::handleData(QByteArray &data, bool *keep)
{
	QByteArray buffer;
	qint64 toRead, toReadInit;
	char c;

	Q_UNUSED(data); // it contains only "GETFILE:"

	if (!continuation) {
		// GETFILE:file[len]=
		if (session->bytesAvailable() < file.length() + 4)
			return -1;

		QByteArray arr = session->read(file.length());
		if (arr != file) {
			qWarning() << "unexpected" << arr;
			return -1;
		}

		if (!session->getChar(&c) || c != '[')
			return -1;

		bool done = false;
		while (session->bytesAvailable()) {
			if (!session->getChar(&c)) {
				qWarning() << "socket.getChar failed";
				continue;
			}

			if (c == ']') {
				done = true;
				break;
			}

			buffer.append(c);
		}

		if (!done)
			return -1;

		if (!session->getChar(&c) || c != '=')
			return -1;

		toRead = buffer.toInt(&done);
		if (!done)
			return -1;
	} else
		toRead = continuation;

	toReadInit = toRead;
	qint64 avail = session->bytesAvailable();

	/*qDebug() << QTime::currentTime() << "avail:" << avail <<
	            "toread:" << toRead;*/

	if (avail < toRead)
		toRead = avail;

	if (toRead)
		fileBuffer.append(session->read(toRead));

	continuation = toReadInit - toRead;
	if (continuation) {
		qDebug() << "not enough bytes in the buffer:" <<
		            avail << "need:" << toReadInit << "requesting more:" <<
		            continuation;
		data.clear();
		return continuation;
	}

	while (session->bytesAvailable()) {
		if (!session->getChar(&c)) {
			qWarning() << "socket.getChar failed";
			continue;
		}

		if (c == '\r')
			continue;
		if (c == '\n')
			break;

		return -1;
	}


	if (toReadInit == 0) {
		callbackFn(this, fileBuffer);
		emit done();
		return 0;
	}

	// we will receive GETFILE:file[0]=
	*keep = true;
	return 0;
}
