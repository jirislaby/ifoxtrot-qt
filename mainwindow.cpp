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
    const QSet<QByteArray>LISTSkip {"LIST:__PF_CRC,DWORD\r\n","LIST:__PLC_RUN,BOOL\r\n"};
    statusBar()->showMessage("Connected");
    state = Connected;

    socket.write("LIST:\n");

    QRegularExpression LISTRE("^LIST:(.+)\\.GTSAP1_([^_]+)_(.+),(.+)\r\n$");
    QRegularExpression GETRE("^GET:(.+)\\.GTSAP1_([^_]+)_(.+),(.+)\r\n$");
    QStringList list;
    FoxtrotItem *item = nullptr;

    while (1) {
        if (!socket.waitForReadyRead(5000)) {
            socket.abort();
            return;
        }

        while (socket.canReadLine()) {
            QByteArray lineArray = socket.readLine();
            if (LISTSkip.contains(lineArray))
                continue;
            if (lineArray == "LIST:\r\n")
                goto done;

            QString line = QString::fromLatin1(lineArray.data());
            QRegularExpressionMatch match = LISTRE.match(line);
            if (!match.hasMatch()) {
                qDebug() << "wrong line" << line;
                continue;
            }
            QString foxName = match.captured(1);
            QString foxType = match.captured(2);
            QString prop = match.captured(3);
            QString propType = match.captured(4);
            if (!item || item->getFoxName() != foxName) {
                //item = new FoxtrotItem(FoxtrotItem::getType(foxType), foxName);
                item = new FoxtrotItem(foxType, foxName);
                items.insert(foxName, item);
            }
            item->setProp(prop, QVariant(0));
            qDebug() << foxName << foxType << prop << propType;
        }
    }

done:
    QTextCodec *codec = QTextCodec::codecForName("Windows 1250");

    for (auto a : items) {
        QByteArray query("GET:");
        query.append(a->getFoxName()).append(".GTSAP1_").append(a->getFoxType()).append("_NAME\n");
        socket.write(query);
        if (!socket.waitForReadyRead(5000)) {
            socket.abort();
            return;
        }

        QByteArray lineArray = socket.readLine();
        QString reply = codec->toUnicode(lineArray.split(',')[1]);
        qDebug() << lineArray << reply;
        reply.chop(3);
        list.append(a->getFoxType()[0] + " " + reply.mid(1));
    }
    list.sort();
    model->setStringList(list);

}

void MainWindow::disconnected()
{
    statusBar()->showMessage("Disconnected");
    ui->butConnect->setText("&Connect");
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
