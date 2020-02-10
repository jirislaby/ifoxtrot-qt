#ifndef IFOXTROTCONN_H
#define IFOXTROTCONN_H

#include <QMap>
#include <QObject>
#include <QQueue>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QTimer>

#include "ifoxtrotmodel.h"
#include "ifoxtrotreceiver.h"

class iFoxtrotSession;

class iFoxtrotSessionInit : public QObject
{
    Q_OBJECT
public:
    explicit iFoxtrotSessionInit(iFoxtrotSession *session,
                                 QObject *parent = nullptr);

    QString getPLCVersion() const { return PLCVersion; }
    void setModelList(iFoxtrotModel &model) const { model.setList(list); }

signals:
    void connected();
    void error(QAbstractSocket::SocketError socketError);
    void statusUpdate(const QString &status);

public slots:
    void sockReadyRead();
    void timeout();

private:
    enum ConPhase {
        PhGetinfo,
        PhGetEnable,
        PhGet,
        PhDone,
    };

    iFoxtrotSession *session;
    enum ConPhase phase;
    QString PLCVersion;
    QByteArray enableString;
    QList<iFoxtrotCtl *> list;
    QRegularExpression GETRE;
    QTextCodec *codec;
    QTimer timer;
    const unsigned int sockTimeout = 15000;

    bool receive(const QString &req,
                 const std::function<void(const QString &)> &fun);
};

class iFoxtrotSession : public QObject
{
    Q_OBJECT
public:
    enum ConState {
        Disconnected,
        Connecting,
        Connected,
    };

    typedef QMap<QString, iFoxtrotCtl *> ItemsFox;
    explicit iFoxtrotSession(QObject *parent = nullptr);

    iFoxtrotModel *getModel() { return &model; }
    const iFoxtrotModel *getModel() const { return &model; }
    enum ConState getState() const { return state; }
    QString getPLCVersion() const { return PLCVersion; }

    void connectToHost(const QString &host, quint16 port) {
        socket.connectToHost(host, port);
        state = Connecting;
    }
    void close() {
        socket.close();
        state = Disconnected;
    }
    void abort() {
        socket.abort();
        state = Disconnected;
    }

    bool getChar(char *c) { return socket.getChar(c); }
    qint64 read(char *data, qint64 maxSize) { return socket.read(data, maxSize); }
    QByteArray read(qint64 maxSize) { return socket.read(maxSize); }
    void write(const QByteArray &array) { socket.write(array); }
    qint64 bytesAvailable() const { return socket.bytesAvailable(); }
    bool canReadLine() const { return socket.canReadLine(); }
    QByteArray readLine() { return socket.readLine(); }
    QString getPeerName() const { return socket.peerName(); }

	void enqueueRcv(iFoxtrotReceiver *rcv);
    void receiveFile(const QString &file,
                     const std::function<void(const QByteArray &)> &fun);

    void itemsFoxInsert(const QString &foxName, iFoxtrotCtl *item) {
        itemsFox.insert(foxName, item);
    }
    ItemsFox::const_iterator itemsFoxFind(const QString &foxName) const {
        return itemsFox.find(foxName);
    }
    ItemsFox::const_iterator itemsFoxBegin() const {
        return itemsFox.begin();
    }
    ItemsFox::const_iterator itemsFoxEnd() const {
        return itemsFox.end();
    }

signals:
    void connected();
    void disconnected();
    void error(QAbstractSocket::SocketError socketError);
    void conStatusUpdate(const QString &status);

public slots:
    void sockConnected();
    void sockDisconnected();
    void sockError(QAbstractSocket::SocketError socketError);
    void sockReadyRead();
    void initConnected();
    void initStatusUpdate(const QString &status);
    void initSockError(QAbstractSocket::SocketError socketError);

private:

    iFoxtrotModel model;
    ItemsFox itemsFox;
    QQueue<iFoxtrotReceiver *> toSend;
    QTcpSocket socket;
    enum ConState state;
    QString PLCVersion;
    iFoxtrotReceiverDIFF DIFFrcv;
    iFoxtrotReceiver *contReceiver;
    iFoxtrotReceiver *curReceiver;

    friend class iFoxtrotSessionInit;
};

#endif // IFOXTROTCONN_H
