#ifndef IFOXTROTCONN_H
#define IFOXTROTCONN_H

#include <QObject>
#include <QRegularExpression>
#include <QTcpSocket>

#include "ifoxtrotmodel.h"

class iFoxtrotSession : public QObject
{
    Q_OBJECT
public:
    enum ConState {
        Disconnected,
        Connecting,
        Connected
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

    void receive(const QString &req,
                 const std::function<void(const QString &)> &fun);
    void write(const QByteArray &array) { socket.write(array); }

signals:
    void connected();
    void disconnected();
    void error(QAbstractSocket::SocketError socketError);

public slots:
    void sockConnected();
    void sockDisconnected();
    void sockError(QAbstractSocket::SocketError socketError);
    void sockReadyRead();

private:
    iFoxtrotModel model;
    QMap<QString, iFoxtrotCtl *> itemsFox;
    QTcpSocket socket;
    enum ConState state;
    QString PLCVersion;
    QRegularExpression GETRE;
    QRegularExpression DIFFRE;

    void handleDIFF(const QString &line);
};

#endif // IFOXTROTCONN_H
