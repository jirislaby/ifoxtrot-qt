#include <QByteArray>
#include <QRegularExpression>
#include <QSet>
#include <QTcpSocket>
#include <QTextCodec>

#include "ifoxtrotctl.h"
#include "ifoxtrotmodel.h"
#include "ifoxtrotreceiver.h"
#include "ifoxtrotsession.h"

void iFoxtrotSession::addItem(const QString &foxName,
                              const QString &foxType,
                              const QString &prop,
                              const QString &value,
                              QList<iFoxtrotCtl *> *listFox,
                              QByteArray *enableString)
{
	if (value != "1")
		return;

	iFoxtrotCtl *item = iFoxtrotCtl::getOne(this, foxType, foxName);
	if (!item) {
		qWarning() << "unsupported type" << foxType << "for" << foxName;
		return;
	}
	listFox->append(item);
	itemsFox.insert(foxName, item);
    QString en("EN:");
    en.append(foxName).append(".GTSAP1_").append(foxType).append("_*\n");
    enableString->append(en.toUtf8());

	Q_UNUSED(prop);
	// qDebug() << foxName << foxType << prop << value;
}
void iFoxtrotSession::updateItem(const QString &foxName,
                                 const QString &foxType,
                                 const QString &prop,
                                 const QString &value)
{
	Q_UNUSED(foxType);

	auto itemIt = itemsFoxFind(foxName);
	if (itemIt == itemsFoxEnd()) {
		qWarning() << "cannot find" << foxName << "in items";
		return;
	}
	iFoxtrotCtl *item = itemIt.value();
	item->setProp(prop, value);
}

iFoxtrotSession::iFoxtrotSession(QObject *parent) :
    QObject(parent), state(Disconnected),
    PLCAddr(""),
    DIFFrcv(this),
    contReceiver(nullptr),
    curReceiver(nullptr)
{
    connect(&socket, &QTcpSocket::connected, this, &iFoxtrotSession::sockConnected);
    connect(&socket, &QTcpSocket::disconnected, this, &iFoxtrotSession::sockDisconnected);
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    connect(&socket, &QAbstractSocket::errorOccurred,
            this, &iFoxtrotSession::lowSockError);
#elif QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &iFoxtrotSession::lowSockError);
#else
    connect(&socket, static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            this, &iFoxtrotSession::lowSockError);
#endif
}

iFoxtrotSession::~iFoxtrotSession()
{
	for (auto it = itemsFoxBegin(); it != itemsFoxEnd(); ++it)
		delete *it;
}

void iFoxtrotSession::sockConnected()
{
    state = Connected;
    connect(&socket, &QTcpSocket::readyRead, this, &iFoxtrotSession::sockReadyRead);

    if (!PLCAddr.isEmpty()) {
        QString setconf("SETCONF:ipaddr,");
	    setconf.append(PLCAddr).append('\n');
        auto SETCONFrcv = new iFoxtrotReceiverSETCONF(this, setconf.toUtf8());
	    enqueueRcv(SETCONFrcv);
    }

    QByteArray lineArray = socket.readLine();
    auto GETINFOrcv = new iFoxtrotReceiverGETINFO(this);
    connect(GETINFOrcv, &iFoxtrotReceiver::done, [this, GETINFOrcv] {
	    emit conStatusUpdate("Got info, receiving items");
	    PLCVersion = GETINFOrcv->getPLCVersion();
	    GETINFOrcv->deleteLater();
    });
    enqueueRcv(GETINFOrcv);

    auto listFox = new QList<iFoxtrotCtl *>();
    auto enableString = new QByteArray("DI:\n");

    auto GETrcv = new iFoxtrotReceiverGET(this, "DI:\n"
                                                "EN:*_ENABLE\n"
                                                "GET:\n",
				  [this, listFox, enableString](const QString &foxName,
                                          const QString &foxType,
                                          const QString &prop,
                                          const QString &value) {
	    addItem(foxName, foxType, prop, value, listFox, enableString);
    });
    connect(GETrcv, &iFoxtrotReceiver::done, [this, GETrcv, listFox, enableString] {
	    emit conStatusUpdate("Received items, receiving states");
	    model.setList(*listFox);
        model.sort(0);
        delete listFox;
	    GETrcv->deleteLater();
	    enableString->append("GET:\n");
	    auto GETrcv2 = new iFoxtrotReceiverGET(this, *enableString,
				  [this](const QString &foxName,
					   const QString &foxType,
					   const QString &prop,
					   const QString &value) {
		    updateItem(foxName, foxType, prop, value);
	    });
	    connect(GETrcv2, &iFoxtrotReceiver::done, [this, GETrcv2, enableString] {
		    GETrcv2->deleteLater();
		    delete enableString;
			for (auto it = itemsFoxBegin(); it != itemsFoxEnd(); ++it)
				(*it)->postReceive();
		    model.sort(0);
		    emit connected();
	    });
	    enqueueRcv(GETrcv2);
    });
    enqueueRcv(GETrcv);
}

void iFoxtrotSession::sockDisconnected()
{
    disconnect(&socket, &QTcpSocket::readyRead, this, &iFoxtrotSession::sockReadyRead);
    toSend.clear();
    sockData.clear();
    curReceiver = nullptr;
    state = Disconnected;
    model.clear();
    emit disconnected();
}

void iFoxtrotSession::lowSockError(QAbstractSocket::SocketError socketError)
{
    state = Disconnected;
    emit sockError(socketError);
}

void iFoxtrotSession::sockReadyRead()
{
	bool error = false;

	while (socket.bytesAvailable()) {
		qint64 ret = -1;
		bool isDIFF = false, keep = false;

		if (contReceiver) {
			ret = contReceiver->handleData(sockData, &keep);
		} else {
			char c;
			if (!socket.getChar(&c)) {
				qWarning() << "socket.getChar failed";
				continue;
			}

			if (error && c == '\n') {
				qWarning() << __PRETTY_FUNCTION__ <<
					      "unexpected line received" << sockData;
				sockData.clear();
				error = false;
				continue;
			}

			sockData.append(c);

			if (c != ':')
				continue;

			if (sockData == "ERROR:") {
				sockData = socket.readLine();
				sockData.chop(2); // \r\n
				if (curReceiver) {
					if (curReceiver->handleError(sockData))
						continue;
					ret = 0;
				}
			} else {
				isDIFF = sockData == "DIFF:";
				iFoxtrotReceiver *rcv = isDIFF ? &DIFFrcv : curReceiver;
				//qDebug() << "rcv" << data;
				if (!rcv || rcv->getPrefix() != sockData) {
					qWarning() << "no receiver for" << sockData;
					error = true;
					continue;
				}
				ret = rcv->handleData(sockData, &keep);
				if (ret > 0)
					contReceiver = rcv;
			}
		}

		if (ret == 0) {
			contReceiver = nullptr;
			sockData.clear();
			if (!isDIFF && !keep) {
				if (toSend.empty()) {
					qDebug() << "finished, all done";
					curReceiver = nullptr;
				} else {
					curReceiver = toSend.dequeue();
					qDebug() << "finished, handling" << curReceiver->getWrite().left(40);
					socket.write(curReceiver->getWrite());
				}
			}
		} else if (ret > 0) {
			qDebug() << contReceiver;
		} else {
			contReceiver = nullptr;
			error = true;
		}
	}
}

void iFoxtrotSession::enqueueRcv(iFoxtrotReceiver *rcv)
{
	if (!curReceiver) {
		qDebug() << __func__ << "directly" << rcv->getWrite().left(40);
		curReceiver = rcv;
		socket.write(rcv->getWrite());
		return;
	}

	qDebug() << __func__ << "enqueuing" << rcv->getWrite().left(40);
	toSend.enqueue(rcv);
}

void iFoxtrotSession::receiveFile(const QString &file,
              const std::function<void(const QByteArray &)> &fun)
{
    QString req("GETFILE:");
	req.append(file).append('\n');

    enqueueRcv(new iFoxtrotReceiverFile(this, req.toUtf8(), file,
		[fun, req](iFoxtrotReceiverFile *frf,
				const QByteArray &data) -> void {
			fun(data);
			frf->deleteLater();
	}));
}
