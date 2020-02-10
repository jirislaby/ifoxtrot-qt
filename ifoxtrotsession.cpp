#include <QByteArray>
#include <QRegularExpression>
#include <QSet>
#include <QTcpSocket>
#include <QTextCodec>

#include "ifoxtrotctl.h"
#include "ifoxtrotmodel.h"
#include "ifoxtrotreceiver.h"
#include "ifoxtrotsession.h"

iFoxtrotSessionInit::iFoxtrotSessionInit(iFoxtrotSession *session,
                                         QObject *parent) :
    QObject(parent), session(session), phase(PhGetinfo), enableString("DI:\n"),
    GETRE("^GET:(.+)\\.GTSAP1_([^_]+)_(.+),(.+)\r\n$"),
    codec(QTextCodec::codecForName("Windows 1250"))
{
    connect(&timer, &QTimer::timeout, this,
            QOverload<>::of(&iFoxtrotSessionInit::timeout));
    timer.setSingleShot(true);
    timer.start(sockTimeout);
}

void iFoxtrotSessionInit::timeout()
{
    emit error(QAbstractSocket::SocketTimeoutError);
}

void iFoxtrotSessionInit::sockReadyRead()
{
    timer.start(sockTimeout);

    switch (phase) {
    case PhGetinfo:
        if (receive("GETINFO", [this](const QString &line) -> void {
                    if (line.startsWith("GETINFO:VERSION_PLC,")) {
                        PLCVersion = line.mid(sizeof("GETINFO:VERSION_PLC,") - 1);
                        PLCVersion.chop(2); // \r\n
                    }
                })) {

            session->write("DI:\n"
                     "EN:*_ENABLE\n"
                     "GET:\n");

            emit statusUpdate("Got info, receiving items");
            phase = PhGetEnable;
        }
        break;
    case PhGetEnable:
        if (receive("GET", [this](const QString &line) -> void {
                    QRegularExpressionMatch match = GETRE.match(line);
                    if (!match.hasMatch()) {
                        qDebug() << "wrong GET line" << line;
                        return;
                    }

                    QString foxName = match.captured(1);
                    QString foxType = match.captured(2);
                    QString prop = match.captured(3);
                    QString value = match.captured(4);
                    if (value != "1")
                        return;

                    iFoxtrotCtl *item = iFoxtrotCtl::getOne(session, foxType,
                                                            foxName);
                    if (!item) {
                        qWarning() << "unsupported type" << foxType << "for" << foxName;
                        return;
                    }
                    list.append(item);
                    session->itemsFoxInsert(foxName, item);
                    enableString.append("EN:").append(foxName).append(".GTSAP1_").append(foxType).append("_*\n");
                    //qDebug() << foxName << foxType << prop << value;
                })) {

            enableString.append("GET:\n");
            session->write(enableString);

            emit statusUpdate("Received items, receiving states");
            phase = PhGet;
        }
        break;
    case PhGet:
        if (receive("GET", [this](const QString &line) -> void {
                    QRegularExpressionMatch match = GETRE.match(line);
                    if (!match.hasMatch()) {
                        qDebug() << "wrong GET line" << line;
                        return;
                    }
                    QString foxName = match.captured(1);
                    QString foxType = match.captured(2);
                    QString prop = match.captured(3);
                    QString value = match.captured(4);
                    iFoxtrotSession::ItemsFox::const_iterator itemIt =
                            session->itemsFoxFind(foxName);
                    if (itemIt == session->itemsFoxEnd()) {
                        qWarning() << "cannot find" << foxName << "in items";
                        return;
                    }
                    iFoxtrotCtl *item = itemIt.value();
                    item->setProp(prop, value);
                })) {

			for (iFoxtrotSession::ItemsFox::const_iterator it = session->itemsFoxBegin();
					it != session->itemsFoxEnd(); ++it) {
				(*it)->postReceive();
			}
            emit connected();
            phase = PhDone;
        }
        break;
    case PhDone:
        qWarning() << __PRETTY_FUNCTION__ << "superfluous call";
        break;
    }

}

bool iFoxtrotSessionInit::receive(const QString &req,
                              const std::function<void(const QString &)> &fun)
{
    static const QSet<QByteArray>skip {
        "__PF_CRC",
        "__PLC_RUN"
    };
    const QByteArray requestColon = req.toLatin1() + ":";
    const QByteArray requestEOL = requestColon + "\r\n";
    const unsigned reqSize = requestColon.size();

    while (session->canReadLine()) {
        QByteArray lineArray = session->readLine();

        if (lineArray.startsWith("DIFF:")) {
            iFoxtrotReceiverDIFF::handleDIFF(session,
	                                     codec->toUnicode(lineArray.data()));
            continue;
        }

        if (!lineArray.startsWith(requestColon)) {
            qWarning() << __PRETTY_FUNCTION__ << "unexpected line received" <<
                          lineArray << "expected" << requestColon <<
                          "phase" << phase;
            continue;
        }

        QByteArray crop = lineArray.mid(reqSize);
        crop.truncate(crop.indexOf(','));
        if (skip.contains(crop))
            continue;

        if (lineArray == requestEOL)
            return true;

        QString line = codec->toUnicode(lineArray.data());
        fun(line);
    }

    return false;
}

iFoxtrotSession::iFoxtrotSession(QObject *parent) :
    QObject(parent), state(Disconnected),
    DIFFrcv(this),
    contReceiver(nullptr),
    curReceiver(nullptr)
{
    connect(&socket, &QTcpSocket::connected, this, &iFoxtrotSession::sockConnected);
    connect(&socket, &QTcpSocket::disconnected, this, &iFoxtrotSession::sockDisconnected);
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &iFoxtrotSession::sockError);
#else
    connect(&socket, static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            this, &iFoxtrotSession::sockError);
#endif
}

void iFoxtrotSession::sockConnected()
{
    static const QSet<QByteArray>skip {
        "__PF_CRC",
        "__PLC_RUN"
    };
    state = Connected;

    auto sesInit = new iFoxtrotSessionInit(this, this);

    connect(&socket, &QTcpSocket::readyRead, sesInit,
            &iFoxtrotSessionInit::sockReadyRead);
    connect(sesInit, &iFoxtrotSessionInit::connected, this,
            &iFoxtrotSession::initConnected);
    connect(sesInit, &iFoxtrotSessionInit::error, this,
            &iFoxtrotSession::initSockError);
    connect(sesInit, &iFoxtrotSessionInit::statusUpdate, this,
            &iFoxtrotSession::conStatusUpdate);

#ifdef SETCONF
    QString setconf("SETCONF:ipaddr,");
    setconf.append(ui->lineEditPLCAddr->text()).append('\n');
    socket.write(setconf);
    if (!socket.waitForReadyRead(5000)) {
        socket.abort();
        return;
    }
#endif

    QByteArray lineArray = socket.readLine();
    socket.write("GETINFO:\n");
}

void iFoxtrotSession::initConnected()
{
    auto sesInit = dynamic_cast<iFoxtrotSessionInit *>(sender());

    connect(&socket, &QTcpSocket::readyRead, this, &iFoxtrotSession::sockReadyRead);

    PLCVersion = sesInit->getPLCVersion();

    sesInit->setModelList(model);
    model.sort(0);

    sesInit->deleteLater();

    emit connected();
}

void iFoxtrotSession::initStatusUpdate(const QString &status)
{
    emit conStatusUpdate(status);
}

void iFoxtrotSession::initSockError(QAbstractSocket::SocketError socketError)
{
    auto sesInit = dynamic_cast<iFoxtrotSessionInit *>(sender());

    sesInit->deleteLater();

    abort();
    emit error(socketError);
}

void iFoxtrotSession::sockDisconnected()
{
    disconnect(&socket, &QTcpSocket::readyRead, this, &iFoxtrotSession::sockReadyRead);
    toSend.clear();
    curReceiver = nullptr;
    state = Disconnected;
    model.clear();
    emit disconnected();
}

void iFoxtrotSession::sockError(QAbstractSocket::SocketError socketError)
{
    state = Disconnected;
    emit error(socketError);
}

void iFoxtrotSession::sockReadyRead()
{
	QByteArray data;
	bool error = false;

	while (socket.bytesAvailable()) {
		qint64 ret = -1;
		bool isDIFF = false, keep = false;

		if (contReceiver) {
			ret = contReceiver->handleData(data, &keep);
		} else {
			char c;
			if (!socket.getChar(&c)) {
				qWarning() << "socket.getChar failed";
				continue;
			}

			if (error && c == '\n') {
				qWarning() << __PRETTY_FUNCTION__ <<
					      "unexpected line received" << data;
				data.clear();
				error = false;
				continue;
			}

			data.append(c);

			if (c != ':')
				continue;

			if (data == "ERROR:") {
				data = socket.readLine();
				data.chop(2); // \r\n
				if (curReceiver) {
					if (curReceiver->handleError(data))
						continue;
					ret = 0;
				}
			} else {
				isDIFF = data == "DIFF:";
				iFoxtrotReceiver *rcv = isDIFF ? &DIFFrcv : curReceiver;
				//qDebug() << "rcv" << data;
				if (!rcv || rcv->getPrefix() != data) {
					qWarning() << "no receiver for" << data;
					error = true;
					continue;
				}
				ret = rcv->handleData(data, &keep);
				if (ret > 0)
					contReceiver = rcv;
			}
		}

		if (ret == 0) {
			contReceiver = nullptr;
			data.clear();
			if (!isDIFF && !keep) {
				if (toSend.empty()) {
					qDebug() << "finished, all done";
					curReceiver = nullptr;
				} else {
					curReceiver = toSend.dequeue();
					qDebug() << "finished, handling" << curReceiver->getWrite();
					socket.write(curReceiver->getWrite());
				}
			}
		} else if (ret > 0) {
			qDebug() << contReceiver;
			data.clear();
		} else {
			contReceiver = nullptr;
			error = true;
		}
	}
}

void iFoxtrotSession::enqueueRcv(iFoxtrotReceiver *rcv)
{
	if (!curReceiver) {
		qDebug() << __func__ << "directly" << rcv->getWrite();
		curReceiver = rcv;
		socket.write(rcv->getWrite());
		return;
	}

	qDebug() << __func__ << "enqueuing" << rcv->getWrite();
	toSend.enqueue(rcv);
}

void iFoxtrotSession::receiveFile(const QString &file,
              const std::function<void(const QByteArray &)> &fun)
{
	QByteArray req("GETFILE:");
	req.append(file).append('\n');

	enqueueRcv(new iFoxtrotReceiverFile(this, req, file,
		[fun, req](iFoxtrotReceiverFile *frf,
				const QByteArray &data) -> void {
			fun(data);
			frf->deleteLater();
	}));
}
