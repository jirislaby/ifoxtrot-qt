#ifndef IFOXTROTRECEIVER_H
#define IFOXTROTRECEIVER_H

#include <QObject>

class iFoxtrotReceiver : public QObject
{
    Q_OBJECT
public:
    explicit iFoxtrotReceiver(QObject *parent = nullptr);

signals:

public slots:
};

#endif // IFOXTROTRECEIVER_H
