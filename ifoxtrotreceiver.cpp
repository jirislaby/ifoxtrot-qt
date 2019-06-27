#include <QDebug>
#include <QTextCodec>

#include "ifoxtrotreceiver.h"
#include "ifoxtrotsession.h"

iFoxtrotReceiver::iFoxtrotReceiver(iFoxtrotSession *session, QObject *parent) :
        QObject(parent), session(session), byLines(true)
{
}

bool iFoxtrotReceiver::handleData(QByteArray &data)
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
			QTextCodec *codec = QTextCodec::codecForName("Windows 1250");
			emit hasData(codec->toUnicode(data.data()));
			return true;
		}

		data.append(c);
	}

	return false;
}

iFoxtrotReceiverFile::iFoxtrotReceiverFile(iFoxtrotSession *session,
                                           const QString &file,
                                           const CallbackFn &fun,
                                           QObject *parent) :
        iFoxtrotReceiver(session, parent), file(file), callbackFn(fun)
{
	byLines = false;
}

bool iFoxtrotReceiverFile::handleData(QByteArray &data)
{
	QByteArray buffer;
	char c;

	// GETFILE:file[len]=
	if (session->bytesAvailable() < file.length() + 4)
		return false;

	if (session->read(file.length()) != file)
		return false;

	if (!session->getChar(&c) || c != '[')
		return false;

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
		return false;

	if (!session->getChar(&c) || c != '=')
		return false;

	qint64 toRead = buffer.toInt(&done);
	if (!done)
		return false;

	if (session->bytesAvailable() < toRead)
		return false;

	if (toRead)
		fileBuffer.append(session->read(toRead));

	while (session->bytesAvailable()) {
		if (!session->getChar(&c)) {
			qWarning() << "socket.getChar failed";
			continue;
		}

		if (c == '\r')
			continue;
		if (c == '\n')
			break;

		return false;
	}

	if (toRead == 0)
		callbackFn(fileBuffer);

	return true;
}
