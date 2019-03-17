#include <QByteArray>
#include <QRegularExpression>
#include <QSet>
#include <QTcpSocket>
#include <QTextCodec>

#include "ifoxtrotctl.h"
#include "ifoxtrotmodel.h"
#include "ifoxtrotsession.h"

iFoxtrotSession::iFoxtrotSession(QObject *parent) :
    QObject(parent), state(Disconnected),
    GETRE("^GET:(.+)\\.GTSAP1_([^_]+)_(.+),(.+)\r\n$"),
    DIFFRE("^DIFF:(.+)\\.GTSAP1_([^_]+)_(.+),(.+)\r\n$")
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

    qDebug() << __PRETTY_FUNCTION__ << "DIFF" << foxName << foxType << prop << value;
}

void iFoxtrotSession::receive(const QString &req,
                              const std::function<void(const QString &)> &fun)
{
    static const QSet<QByteArray>skip {
        "__PF_CRC",
        "__PLC_RUN"
    };
    QTextCodec *codec = QTextCodec::codecForName("Windows 1250");
    const QByteArray requestColon = req.toLatin1() + ":";
    const QByteArray requestEOL = requestColon + "\r\n";
    const unsigned reqSize = requestColon.size();
    bool done = false;

    while (!done) {
        if (!socket.waitForReadyRead(5000)) {
            emit error(QAbstractSocket::SocketTimeoutError);
            abort();
            return;
        }

        while (socket.canReadLine()) {
            QByteArray lineArray = socket.readLine();

            if (lineArray.startsWith("DIFF:")) {
                handleDIFF(codec->toUnicode(lineArray.data()));
                continue;
            }

            if (!lineArray.startsWith(requestColon)) {
                qWarning() << __PRETTY_FUNCTION__ << "unexpected line received" <<
                              lineArray;
                continue;
            }

            QByteArray crop = lineArray.mid(reqSize);
            crop.truncate(crop.indexOf(','));
            if (skip.contains(crop))
                continue;

            if (lineArray == requestEOL) {
                done = true;
                break;
            }

            QString line = codec->toUnicode(lineArray.data());
            fun(line);
        }
    }
}

void iFoxtrotSession::sockConnected()
{
    const QSet<QByteArray>skip {
        "__PF_CRC",
        "__PLC_RUN"
    };
    state = Connected;

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
    receive("GETINFO", [this](const QString &line) -> void {
            if (line.startsWith("GETINFO:VERSION_PLC,")) {
                PLCVersion = line.mid(sizeof("GETINFO:VERSION_PLC,") - 1);
                PLCVersion.chop(2); // \r\n
            }
            //qDebug() << lineArray;
        });

    QByteArray enableString("DI:\n");
    QList<iFoxtrotCtl *> list;

    socket.write("DI:\n"
                 "EN:*_ENABLE\n"
                 "GET:\n");

    receive("GET", [this, &list, &enableString](const QString &line) -> void {
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

            iFoxtrotCtl *item = iFoxtrotCtl::getOne(this, foxType, foxName);
            if (!item) {
                qWarning() << "unsupported type" << foxType << "for" << foxName;
                return;
            }
            list.append(item);
            itemsFox.insert(foxName, item);
            enableString.append("EN:").append(foxName).append(".GTSAP1_").append(foxType).append("_*\n");
            //qDebug() << foxName << foxType << prop << value;
        });

    enableString.append("GET:\n");
    socket.write(enableString);

    receive("GET", [this](const QString &line) -> void {
            QRegularExpressionMatch match = GETRE.match(line);
            if (!match.hasMatch()) {
                qDebug() << "wrong GET line" << line;
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
        });

    model.setList(list);
    model.sort(0);

    connect(&socket, &QTcpSocket::readyRead, this, &iFoxtrotSession::sockReadyRead);

    emit connected();
}


void iFoxtrotSession::sockDisconnected()
{
    disconnect(&socket, &QTcpSocket::readyRead, this, &iFoxtrotSession::sockReadyRead);
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
    QTextCodec *codec = QTextCodec::codecForName("Windows 1250");

    while (socket.canReadLine()) {
        QByteArray lineArray = socket.readLine();

        if (!lineArray.startsWith("DIFF:")) {
            qWarning() << __PRETTY_FUNCTION__ << "unexpected line received" <<
                          lineArray;
            continue;
        }

        handleDIFF(codec->toUnicode(lineArray.data()));
    }
}
