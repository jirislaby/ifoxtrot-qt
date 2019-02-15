#include <QMessageBox>
#include <QTextCodec>

#include "mainwindow.h"
#include "ui_mainwindow.h"

#if 0
FoxtrotItem::Type FoxtrotItem::getType(QString type)
{
    if (type.startsWith("LIGHT"))
        return FoxtrotItem::Type::Light;
    if (type.startsWith("RELAY"))
        return FoxtrotItem::Type::Relay;
    return FoxtrotItem::Type::Unknown;
}
#endif

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    state(Disconnected),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(&socket, &QTcpSocket::connected, this, &MainWindow::connected);
    connect(&socket, &QTcpSocket::disconnected, this, &MainWindow::disconnected);
//    connect(&socket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);
//    connect(&socket, &QTcpSocket::error, this, &MainWindow::sockError);

    model = new QStringListModel(this);
    ui->listViewItems->setModel(model);
}

MainWindow::~MainWindow()
{
    socket.close();
    delete ui;
}

void MainWindow::on_butConnect_clicked()
{
    switch (state) {
    case Connecting:
        socket.abort();
        return;
    case Connected:
        socket.close();
        return;
    default:
        break;
    }

    QString addr = ui->lineEditAddr->text();
    quint16 port = static_cast<quint16>(ui->spinBoxPort->value());

    if (addr.length() == 0) {
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Critical);
        mbox.setText("Empty address!");
        mbox.exec();
        return;
    }

    socket.connectToHost(addr, port);

    statusBar()->showMessage("Connecting to " + addr + ":" + QString::number(port));
    ui->butConnect->setText("&Disconnect");
    state = Connecting;
}

void MainWindow::connected()
{
    const QSet<QByteArray>skip {
        "__PF_CRC",
        "__PLC_RUN"
    };
    const QSet<QString>supportedTypes {
        "LIGHT",
        "RELAY"
    };
    statusBar()->showMessage("Connected");
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
            socket.abort();
            statusBar()->showMessage("GETINFO failed");
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
                ui->labelPLC->setText(lineArray.mid(sizeof("GETINFO:VERSION_PLC,") - 1));
            }
            //qDebug() << lineArray;
        }
    }

    QTextCodec *codec = QTextCodec::codecForName("Windows 1250");
    QRegularExpression GETRE("^GET:(.+)\\.GTSAP1_([^_]+)_(.+),(.+)\r\n$");
    QByteArray enableString("DI:\n");

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
            if (!supportedTypes.contains(foxType))
                continue;

            FoxtrotItem *item = new FoxtrotItem(foxType, foxName);
            itemsFox.insert(foxName, item);
            enableString.append("EN:").append(foxName).append(".GTSAP1_").append(foxType).append("_*\n");
            //qDebug() << foxName << foxType << prop << value;
        }
    }

    enableString.append("GET:\n");
    socket.write(enableString);
    QStringList list;

    done = false;
    while (!done) {
        if (!socket.waitForReadyRead(5000)) {
            socket.abort();
            return;
        }

        while (socket.canReadLine()) {
            QByteArray lineArray = socket.readLine();
            QByteArray crop = lineArray.mid(sizeof("GET:") - 1, lineArray.indexOf(',') - 4);
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
            QMap<QString, FoxtrotItem *>::const_iterator itemIt = itemsFox.find(foxName);
            if (itemIt == itemsFox.end()) {
                qWarning() << "cannot find" << foxName << "in items";
                continue;
            }
            FoxtrotItem *item = itemIt.value();
            item->setProp(prop, value);
        }
    }

    for (FoxtrotItem *item : itemsFox) {
        if (item->hasProp("NAME")) {
            QString val = item->getProp("NAME").toString();

            val.remove(0, 1);
            val.chop(1);
            val.prepend(item->getFoxType()[0] + " ");
            list.append(val);
            itemsName.insert(val, item);

        }
    }

    list.sort();
    model->setStringList(list);

}

void MainWindow::disconnected()
{
    statusBar()->showMessage("Disconnected");
    ui->butConnect->setText("&Connect");
    ui->labelPLC->setText("");
    state = Disconnected;
}

void MainWindow::readyRead()
{
}

void MainWindow::sockError(QAbstractSocket::SocketError socketError)
{
    (void)socketError;
    disconnected();
    statusBar()->showMessage("Error connecting: " + socket.errorString());
}

void MainWindow::on_listViewItems_clicked(const QModelIndex &index)
{
    QString name = model->data(index, Qt::DisplayRole).toString();
    qDebug() << "clicked" << index.row() << name;
    FoxtrotItem *item = itemsName.value(name);
    for (QMap<QString, QVariant>::const_iterator i = item->begin(); i != item->end(); ++i)
        qDebug() << i.key() << i.value().toString();
}
