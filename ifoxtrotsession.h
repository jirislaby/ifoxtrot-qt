#ifndef IFOXTROTCONN_H
#define IFOXTROTCONN_H

#include <QObject>
#include <QRegularExpression>
#include <QTcpSocket>
#include <QTimer>

#include "ifoxtrotmodel.h"


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

    void write(const QByteArray &array) { socket.write(array); }
    bool canReadLine() const { return socket.canReadLine(); }
    QByteArray readLine() { return socket.readLine(); }
    QString getPeerName() const { return socket.peerName(); }

    void receiveFile(const QString &file,
                     const std::function<void(const QByteArray &)> &fun);

    void itemsFoxInsert(const QString &foxName, iFoxtrotCtl *item) {
        itemsFox.insert(foxName, item);
    }
    QMap<QString, iFoxtrotCtl *>::const_iterator itemsFoxFind(const QString &foxName) const {
        return itemsFox.find(foxName);
    }
    QMap<QString, iFoxtrotCtl *>::const_iterator itemsFoxEnd() const {
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
    QMap<QString, iFoxtrotCtl *> itemsFox;
    QMap<QByteArray, std::function<void(QByteArray &)>> dataHandlers;
    QTcpSocket socket;
    enum ConState state;
    QString PLCVersion;
    QRegularExpression DIFFRE;

    void handleDIFF(const QString &line);

    friend class iFoxtrotSessionInit;
};

#endif // IFOXTROTCONN_H
