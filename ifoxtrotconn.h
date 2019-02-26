#ifndef IFOXTROTCONN_H
#define IFOXTROTCONN_H

#include <QObject>
#include <QTcpSocket>

class iFoxtrotConn : public QObject
{
    Q_OBJECT
public:
    explicit iFoxtrotConn(QObject *parent = nullptr);

signals:

public slots:
private:
    QTcpSocket socket;
};

#endif // IFOXTROTCONN_H
