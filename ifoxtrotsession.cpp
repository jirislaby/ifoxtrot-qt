#include <QByteArray>
#include <QRegularExpression>
#include <QSet>
#include <QTcpSocket>
#include <QTextCodec>

#include "ifoxtrotctl.h"
#include "ifoxtrotmodel.h"
#include "ifoxtrotsession.h"

iFoxtrotSession::iFoxtrotSession(QObject *parent) :
    QObject(parent), state(Disconnected)
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
    bool done = false;
    while (!done) {
        if (!socket.waitForReadyRead(5000)) {
            emit error(QAbstractSocket::SocketTimeoutError);
            abort();
            //statusBar()->showMessage("GETINFO failed");
            return;
        }

        while (socket.canReadLine()) {
            QByteArray lineArray = socket.readLine();
            if (lineArray == "GETINFO:\r\n") {
                done = true;
                break;
            }
            if (lineArray.startsWith("GETINFO:VERSION_PLC,")) {
                lineArray.chop(2); // \r\n
                PLCVersion = lineArray.mid(sizeof("GETINFO:VERSION_PLC,") - 1);
            }
            //qDebug() << lineArray;
        }
    }

    QTextCodec *codec = QTextCodec::codecForName("Windows 1250");
    QRegularExpression GETRE("^GET:(.+)\\.GTSAP1_([^_]+)_(.+),(.+)\r\n$");
    QByteArray enableString("DI:\n");
    QMap<QString, iFoxtrotCtl *> itemsFox;
    QList<iFoxtrotCtl *> list;

    socket.write("DI:\n"
                 "EN:*_ENABLE\n"
                 "GET:\n");

    done = false;
    while (!done) {
        if (!socket.waitForReadyRead(5000)) {
            socket.abort();
            return;
        }

        while (socket.canReadLine()) {
            QByteArray lineArray = socket.readLine();
            QByteArray crop = lineArray.mid(sizeof("GET:") - 1);
            crop.truncate(crop.indexOf(','));
            if (skip.contains(crop))
                continue;
            if (lineArray == "GET:\r\n") {
                done = true;
                break;
            }

            QString line = codec->toUnicode(lineArray.data());
            QRegularExpressionMatch match = GETRE.match(line);
            if (!match.hasMatch()) {
                qDebug() << "wrong GET line" << line << crop;
                continue;
            }
            QString foxName = match.captured(1);
            QString foxType = match.captured(2);
            QString prop = match.captured(3);
            QString value = match.captured(4);
            if (value != "1")
                continue;

            iFoxtrotCtl *item = iFoxtrotCtl::getOne(this, foxType, foxName);
            if (!item) {
                qWarning() << "unsupported type" << foxType << "for" << foxName;
                continue;
            }
            list.append(item);
            itemsFox.insert(foxName, item);
            enableString.append("EN:").append(foxName).append(".GTSAP1_").append(foxType).append("_*\n");
            //qDebug() << foxName << foxType << prop << value;
        }
    }

    enableString.append("GET:\n");
    socket.write(enableString);

    done = false;
    while (!done) {
        if (!socket.waitForReadyRead(5000)) {
            socket.abort();
            return;
        }

        while (socket.canReadLine()) {
            QByteArray lineArray = socket.readLine();
            QByteArray crop = lineArray.mid(sizeof("GET:") - 1,
                                            lineArray.indexOf(',') - 4);
            if (skip.contains(crop))
                continue;
            if (lineArray == "GET:\r\n") {
                done = true;
                break;
            }

            QString line = codec->toUnicode(lineArray.data());
            QRegularExpressionMatch match = GETRE.match(line);
            if (!match.hasMatch()) {
                qDebug() << "wrong GET line" << line << crop;
                continue;
            }
            QString foxName = match.captured(1);
            QString foxType = match.captured(2);
            QString prop = match.captured(3);
            QString value = match.captured(4);
            QMap<QString, iFoxtrotCtl *>::const_iterator itemIt = itemsFox.find(foxName);
            if (itemIt == itemsFox.end()) {
                qWarning() << "cannot find" << foxName << "in items";
                continue;
            }
            iFoxtrotCtl *item = itemIt.value();
            item->setProp(prop, value);
        }
    }

    model.setList(list);
    model.sort(0);

    connect(&socket, &QTcpSocket::readyRead, this, &iFoxtrotSession::sockReadyRead);
}


void iFoxtrotSession::sockDisconnected()
{
    state = Disconnected;
}

void iFoxtrotSession::sockError(QAbstractSocket::SocketError socketError)
{
    emit error(socketError);
}

void iFoxtrotSession::sockReadyRead()
{
    QRegularExpression DIFFRE("^DIFF:(.+)\\.GTSAP1_([^_]+)_(.+),(.+)\r\n$");
    QTextCodec *codec = QTextCodec::codecForName("Windows 1250");

    while (socket.canReadLine()) {
        QByteArray lineArray = socket.readLine();

        QString line = codec->toUnicode(lineArray.data());
        QRegularExpressionMatch match = DIFFRE.match(line);
        if (!match.hasMatch()) {
            qDebug() << __PRETTY_FUNCTION__ << " unexpected line" << line;
            continue;
        }
        QString foxName = match.captured(1);
        QString foxType = match.captured(2);
        QString prop = match.captured(3);
        QString value = match.captured(4);

        qDebug() << __PRETTY_FUNCTION__ << "DIFF" << foxName << foxType << prop << value;
    }
}
