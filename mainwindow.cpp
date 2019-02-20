#include <QMessageBox>
#include <QTextCodec>
#include <QSettings>

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
//    connect(&socket, &QTcpSocket::readyRead, this, &MainWindow::readyRead);
//    connect(&socket, &QTcpSocket::error, this, &MainWindow::sockError);

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
            QMap<QString, iFoxtrotCtl *>::const_iterator itemIt = itemsFox.find(foxName);
            if (itemIt == itemsFox.end()) {
                qWarning() << "cannot find" << foxName << "in items";
                continue;
            }
            iFoxtrotCtl *item = itemIt.value();
            item->setProp(prop, value);
        }
    }

    for (iFoxtrotCtl *item : itemsFox) {
        if (item->getName() != "") {
            QString val = item->getName();

            val.prepend(item->getFoxType()[0] + " ");
            itemsName.insert(val, item);

        }
    }

    model->setList(list);
    model->sort(0);
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
    iFoxtrotCtl *item = itemsName.value(name);
    for (int i = 0; i < ui->stackedWidget->count(); i++)
        if (ui->stackedWidget->widget(i)->objectName().mid(5) == item->getFoxType()) {
            ui->stackedWidget->setCurrentIndex(i);
            break;
        }

    ui->labelFoxName->setText(name);
    item->setupUI(ui);
    //ui->labelRelayStatus->setText(item->getProp("ONOFF").toString());

    /*    for (QMap<QString, QVariant>::const_iterator i = item->begin(); i != item->end(); ++i)
        qDebug() << i.key() << i.value().toString();*/
}

void MainWindow::on_pushButtonRelay_clicked()
{

}

void MainWindow::on_pushButtonLight_clicked()
{
    iFoxtrotLight *light = dynamic_cast<iFoxtrotLight *>(itemsName.value(ui->labelFoxName->text()));
    QByteArray req("SET:");
    bool onOff = !light->getOnOff();
    req.append(light->getFoxName()).append(".GTSAP1_").append(light->getFoxType()).append("_ONOFF,").append(onOff ? '1' : '0').append('\n');
    light->setOnOff(onOff);
    ui->labelLightStatus->setText(onOff ? "1" : "0");
    qDebug() << "REQ" << req;
    socket.write(req);
}
