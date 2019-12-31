#include <QDebug>
#include <QTime>
#include <QTextCodec>

#include "ifoxtrotreceiver.h"
#include "ifoxtrotsession.h"

iFoxtrotReceiver::iFoxtrotReceiver(iFoxtrotSession *session,
                                   const QByteArray &prefix,
                                   const QByteArray &write) :
        QObject(session), session(session), prefix(prefix), write(write),
        byLines(true)
{
}

qint64 iFoxtrotReceiver::handleData(QByteArray &data, bool *keep)
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
                                           const QByteArray &write,
                                           const QString &file,
                                           const CallbackFn &fun) :
        iFoxtrotReceiver(session, "GETFILE:", write), file(file),
        callbackFn(fun), continuation(0)
{
	byLines = false;
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
		return 0;
	}

	// we will receive GETFILE:file[0]=
	*keep = true;
	return 0;
}
