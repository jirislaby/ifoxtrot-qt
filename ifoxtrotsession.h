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

    using ItemsFox = QMap<QString, iFoxtrotCtl *>;

    explicit iFoxtrotSession(bool full = true, QObject *parent = nullptr);
    ~iFoxtrotSession();

    iFoxtrotModel *getModel() { return &model; }
    const iFoxtrotModel *getModel() const { return &model; }
    enum ConState getState() const { return state; }
    QString getPLCVersion() const { return PLCVersion; }

    void connectToHost(const QString &host, quint16 port, const QString &PLC);
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
    qint64 bytesToWrite() const { return socket.bytesToWrite(); }
    bool canReadLine() const { return socket.canReadLine(); }
    QByteArray readLine() { return socket.readLine(); }
    QString getPeerName() const { return socket.peerName(); }

	void enqueueRcv(iFoxtrotReceiver *rcv);
    void receiveFile(const QString &file,
                     const std::function<void(const QByteArray &)> &fun);

    void handleDIFF(const QString &foxName, const QString &prop,
		    const QString &value);

    ItemsFox::const_iterator itemsFoxFind(const QString &foxName) const {
        return itemsFox.find(foxName);
    }
    ItemsFox::const_iterator itemsFoxBegin() const {
        return itemsFox.begin();
    }
    ItemsFox::const_iterator itemsFoxEnd() const {
        return itemsFox.end();
    }
    QString getSettingsGrp() const;

signals:
    void received();
    void connected();
    void disconnected();
    void sockError(QAbstractSocket::SocketError socketError);
    void conError(const QString &reason);
    void conStatusUpdate(const QString &status);
    void bytesWritten(qint64 bytes);

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

    bool full;
    iFoxtrotModel model;
    ItemsFox itemsFox;
    QQueue<iFoxtrotReceiver *> toSend;
    QMap<QString, QString> namesCache;
    QTcpSocket socket;
    QByteArray sockData;
    enum ConState state;
    QString host;
    quint16 port;
    QString PLCAddr;
    QString PLCVersion;
    iFoxtrotReceiverDIFF DIFFrcv;
    iFoxtrotReceiver *contReceiver;
    iFoxtrotReceiver *curReceiver;
};

#endif // IFOXTROTCONN_H
