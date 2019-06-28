#include <QDebug>
#include <QTime>
#include <QTextCodec>

#include "ifoxtrotreceiver.h"
#include "ifoxtrotsession.h"

iFoxtrotReceiver::iFoxtrotReceiver(iFoxtrotSession *session, QObject *parent) :
        QObject(parent), session(session), byLines(true)
{
}

qint64 iFoxtrotReceiver::handleData(QByteArray &data)
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
			return 0;
		}

		data.append(c);
	}

	return -1;
}

iFoxtrotReceiverFile::iFoxtrotReceiverFile(iFoxtrotSession *session,
                                           const QString &file,
                                           const CallbackFn &fun,
                                           QObject *parent) :
        iFoxtrotReceiver(session, parent), file(file), callbackFn(fun),
        continuation(0)
{
	byLines = false;
}

qint64 iFoxtrotReceiverFile::handleData(QByteArray &data)
{
	QByteArray buffer;
	qint64 toRead, toReadInit;
	char c;

	if (!continuation) {
		// GETFILE:file[len]=
		if (session->bytesAvailable() < file.length() + 4)
			return -1;

		if (session->read(file.length()) != file)
			return -1;

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

	if (toReadInit == 0)
		callbackFn(this, fileBuffer);

	return 0;
}
