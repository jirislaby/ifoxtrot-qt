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
    ~iFoxtrotSession();

    iFoxtrotModel *getModel() { return &model; }
    const iFoxtrotModel *getModel() const { return &model; }
    enum ConState getState() const { return state; }
    QString getPLCVersion() const { return PLCVersion; }

    void connectToHost(const QString &host, quint16 port, const QString &PLC) {
	    PLCAddr = PLC;
        socket.connectToHost(host, port);
        state = Connecting;
    }
    void conFailed(const QString &reason) {
	    close();
	    emit conError(reason);
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
    void received();
    void connected();
    void disconnected();
    void sockError(QAbstractSocket::SocketError socketError);
    void conError(const QString &reason);
    void conStatusUpdate(const QString &status);

public slots:
    void sockConnected();
    void sockDisconnected();
    void lowSockError(QAbstractSocket::SocketError socketError);
    void sockReadyRead();

private:
    void addItem(const QString &foxName, const QString &foxType,
                 const QString &prop, const QString &value,
                 QList<iFoxtrotCtl *> *listFox, QByteArray *enableString);
    void updateItem(const QString &foxName, const QString &foxType,
                    const QString &prop, const QString &value);

    iFoxtrotModel model;
    ItemsFox itemsFox;
    QQueue<iFoxtrotReceiver *> toSend;
    QTcpSocket socket;
    QByteArray sockData;
    enum ConState state;
    QString PLCAddr;
    QString PLCVersion;
    iFoxtrotReceiverDIFF DIFFrcv;
    iFoxtrotReceiver *contReceiver;
    iFoxtrotReceiver *curReceiver;
};

#endif // IFOXTROTCONN_H
