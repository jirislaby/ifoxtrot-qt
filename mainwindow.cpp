#include <QMessageBox>
#include <QTextCodec>
#include <QSettings>

#include <QtGlobal>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    state(Disconnected),
    ui(new Ui::MainWindow)
{
    QSettings settings("jirislaby", "ifoxtrot");

    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    // settings
    ui->lineEditPLCAddr->setText(settings.value("PLCaddr").toString());
    ui->lineEditAddr->setText(settings.value("IPaddr").toString());
    ui->spinBoxPort->setValue(settings.value("port", "5010").toInt());
    ui->checkBoxAutocon->setChecked(settings.value("autoconnect", false).toBool());

    connect(&socket, &QTcpSocket::connected, this, &MainWindow::connected);
    connect(&socket, &QTcpSocket::disconnected, this, &MainWindow::disconnected);
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
    connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error),
            this, &MainWindow::sockError);
#else
    connect(&socket, static_cast<void (QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            this, &MainWindow::sockError);
#endif

    model = new iFoxtrotModel();
    ui->listViewItems->setModel(model);

    if (ui->checkBoxAutocon->isChecked())
        emit ui->butConnect->click();
}

MainWindow::~MainWindow()
{
    QSettings settings("jirislaby", "ifoxtrot");

    socket.close();

    settings.setValue("PLCaddr", ui->lineEditPLCAddr->text());
    settings.setValue("IPaddr", ui->lineEditAddr->text());
    settings.setValue("port", ui->spinBoxPort->value());
    settings.setValue("autoconnect", ui->checkBoxAutocon->isChecked());

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

            iFoxtrotCtl *item = iFoxtrotCtl::getOne(foxType, foxName);
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

    model->setList(list);
    model->sort(0);

    connect(&socket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);
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

void MainWindow::sockError(QAbstractSocket::SocketError socketError)
{
    qWarning() << "disconnected" << socketError;
    disconnected();
    statusBar()->showMessage("Error connecting: " + socket.errorString());
}

void MainWindow::on_listViewItems_clicked(const QModelIndex &index)
{
    iFoxtrotCtl *item = model->at(index.row());
    QString name = item->getName();
    qDebug() << "clicked" << index.row() << name;

    for (int i = 0; i < ui->stackedWidget->count(); i++)
        if (ui->stackedWidget->widget(i)->objectName().mid(5) == item->getFoxType()) {
            ui->stackedWidget->setCurrentIndex(i);
            break;
        }

    ui->labelFoxName->setText(name);
    item->setupUI(ui);
}

void MainWindow::on_pushButtonLight_clicked()
{
    QModelIndex index = ui->listViewItems->currentIndex();
    int row = index.row();
    iFoxtrotLight *light = dynamic_cast<iFoxtrotLight *>(model->at(row));
    bool onOff = !light->getOnOff();
    QByteArray req = light->GTSAP("SET", "ONOFF", onOff ? "1" : "0");
    light->setOnOff(onOff);
    emit model->dataChanged(index, index);
    ui->labelLightStatus->setText(onOff ? "1" : "0");
    qDebug() << "REQ" << req;
    socket.write(req);
}

void MainWindow::on_pushButtonRelay_clicked()
{
}

void MainWindow::on_pushButtonSc1_clicked()
{
    int row = ui->listViewItems->currentIndex().row();
    iFoxtrotScene *scene = dynamic_cast<iFoxtrotScene *>(model->at(row));
    socket.write(scene->GTSAP("SET", QString("SET").append('1'), "1"));
}

void MainWindow::on_pushButtonSc2_clicked()
{

}

void MainWindow::on_pushButtonSc3_clicked()
{

}

void MainWindow::on_pushButtonSc4_clicked()
{

}

void MainWindow::on_pushButtonSc5_clicked()
{

}

void MainWindow::on_pushButtonSc6_clicked()
{

}

void MainWindow::on_pushButtonSc7_clicked()
{

}

void MainWindow::on_pushButtonSc8_clicked()
{

}
