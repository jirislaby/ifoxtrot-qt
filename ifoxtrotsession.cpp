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
    timer.start(5000);
}

void iFoxtrotSessionInit::timeout()
{
    emit error(QAbstractSocket::SocketTimeoutError);
}

void iFoxtrotSessionInit::sockReadyRead()
{
    timer.start(5000);

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
                    QMap<QString, iFoxtrotCtl *>::const_iterator itemIt =
                            session->itemsFoxFind(foxName);
                    if (itemIt == session->itemsFoxEnd()) {
                        qWarning() << "cannot find" << foxName << "in items";
                        return;
                    }
                    iFoxtrotCtl *item = itemIt.value();
                    item->setProp(prop, value);
                })) {

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
            session->handleDIFF(codec->toUnicode(lineArray.data()));
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
    DIFFRE("^DIFF:(.+)\\.GTSAP1_([^_]+)_(.+),(.+)\r?\n?$"),
    DIFFrcv(this, this),
    contReceiver(nullptr)
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

void iFoxtrotSession::handleDIFF(const QString &line)
{
    QRegularExpressionMatch match = DIFFRE.match(line);

    if (!match.hasMatch()) {
        qDebug() << __PRETTY_FUNCTION__ << " unexpected DIFF line" << line;
        return;
    }

    QString foxName = match.captured(1);
    QString foxType = match.captured(2);
    QString prop = match.captured(3);
    QString value = match.captured(4);

    QMap<QString, iFoxtrotCtl *>::const_iterator itemIt = itemsFox.find(foxName);
    if (itemIt == itemsFox.end()) {
        qWarning() << "cannot find" << foxName << "in items";
        return;
    }
    iFoxtrotCtl *item = itemIt.value();
    item->setProp(prop, value);

    //qDebug() << __PRETTY_FUNCTION__ << "DIFF" << foxName << foxType << prop << value;
}

void iFoxtrotSession::sockConnected()
{
    static const QSet<QByteArray>skip {
        "__PF_CRC",
        "__PLC_RUN"
    };
    state = Connected;

    auto sesInit = new iFoxtrotSessionInit(this, this);

    connect(&DIFFrcv, &iFoxtrotReceiver::hasData, this,
            &iFoxtrotSession::handleDIFF);
    dataHandlers.insert("DIFF:", &DIFFrcv);

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
    dataHandlers.clear();
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

		if (contReceiver) {
			ret = contReceiver->handleData(data);
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

			DataHandlers::const_iterator it = dataHandlers.find(data);
			if (it != dataHandlers.end()) {
				ret = it.value()->handleData(data);
				if (ret > 0)
					contReceiver = it.value();
			}
		}

		if (ret == 0) {
			contReceiver = nullptr;
			data.clear();
		} else if (ret > 0) {
			qDebug() << contReceiver;
			data.clear();
		} else {
			contReceiver = nullptr;
			error = true;
		}
	}
}

void iFoxtrotSession::receiveFile(const QString &file,
              const std::function<void(const QByteArray &)> &fun)
{
	QByteArray req("GETFILE:");
	iFoxtrotReceiverFile *frf = new iFoxtrotReceiverFile(this, file,
		[this, fun, req](iFoxtrotReceiverFile *frf,
				const QByteArray &data) -> void {
			dataHandlers.remove(req);
			fun(data);
			frf->deleteLater();
		}, this);
	dataHandlers.insert(req, frf);
	req.append(file).append('\n');
	socket.write(req);
}
