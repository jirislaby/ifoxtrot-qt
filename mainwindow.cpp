#include <QMessageBox>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    state(Disconnected),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(&socket, &QTcpSocket::connected, this, &MainWindow::connected);
    connect(&socket, &QTcpSocket::disconnected, this, &MainWindow::disconnected);
    connect(&socket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);

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
    statusBar()->showMessage("Connected");
    state = Connected;

    socket.write("LIST:\n");
}

void MainWindow::disconnected()
{
    statusBar()->showMessage("Disconnected");
    ui->butConnect->setText("&Connect");
    state = Disconnected;
}

void MainWindow::readyRead()
{
    QStringList list;
    while (socket.canReadLine()) {
        QByteArray line = socket.readLine();
        if (!line.startsWith("LIST:") || !line.endsWith("\r\n")) {
            qDebug() << "wrong line" << line.startsWith("LIST:") << line.endsWith("\r\n") << line;
            continue;
        }
        line.chop(2);
        line = line.right(line.length()-5);
        qDebug() << line;
        list.append(line);
    }
    model->setStringList(list);
}
