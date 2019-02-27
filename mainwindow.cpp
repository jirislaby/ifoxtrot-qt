#include <QMessageBox>
#include <QSettings>

#include <QtGlobal>

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
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

    connect(&session, &iFoxtrotSession::connected, this, &MainWindow::connected);
    connect(&session, &iFoxtrotSession::disconnected, this, &MainWindow::disconnected);
    connect(&session, &iFoxtrotSession::error, this, &MainWindow::sockError);

    ui->listViewItems->setModel(session.getModel());
    connect(ui->listViewItems->selectionModel(),
            &QItemSelectionModel::currentRowChanged, this,
            &MainWindow::rowChanged);

    if (ui->checkBoxAutocon->isChecked())
        emit ui->butConnect->click();
}

MainWindow::~MainWindow()
{
    QSettings settings("jirislaby", "ifoxtrot");

    session.close();

    settings.setValue("PLCaddr", ui->lineEditPLCAddr->text());
    settings.setValue("IPaddr", ui->lineEditAddr->text());
    settings.setValue("port", ui->spinBoxPort->value());
    settings.setValue("autoconnect", ui->checkBoxAutocon->isChecked());

    delete ui;
}

void MainWindow::on_butConnect_clicked()
{
    switch (session.getState()) {
    case iFoxtrotSession::Connecting:
        session.abort();
        return;
    case iFoxtrotSession::Connected:
        session.close();
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

    session.connectToHost(addr, port);

    statusBar()->showMessage("Connecting to " + addr + ":" + QString::number(port));
    ui->butConnect->setText("&Disconnect");
}

void MainWindow::connected()
{
    statusBar()->showMessage("Connected");
    ui->labelPLC->setText(session.getPLCVersion());
}

void MainWindow::disconnected()
{
    statusBar()->showMessage("Disconnected");
    ui->butConnect->setText("&Connect");
    ui->labelPLC->setText("");
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::readyRead()
{
}

void MainWindow::sockError(QAbstractSocket::SocketError socketError)
{
    qWarning() << "disconnected" << socketError;
    disconnected();
    statusBar()->showMessage("Socket error: " + QString::number(socketError)); // + socket.errorString());
}

void MainWindow::rowChanged(const QModelIndex &current,
                            const QModelIndex &previous)
{
    Q_UNUSED(previous);
    on_listViewItems_clicked(current);
}

void MainWindow::on_listViewItems_clicked(const QModelIndex &index)
{
    iFoxtrotCtl *item = session.getModel()->at(index.row());
    QString name = item->getName();

    for (int i = 0; i < ui->stackedWidget->count(); i++)
        if (ui->stackedWidget->widget(i)->objectName().mid(5) == item->getFoxType()) {
            ui->stackedWidget->setCurrentIndex(i);
            break;
        }

    ui->labelFoxName->setText(name);
    item->setupUI(ui);
}

void MainWindow::on_listViewItems_doubleClicked(const QModelIndex &index)
{
    iFoxtrotCtl *item = session.getModel()->at(index.row());
    qDebug() << "double click" << item->getFoxName();
    item->click();
}

void MainWindow::on_pushButtonLight_clicked()
{
    QModelIndex index = ui->listViewItems->currentIndex();
    int row = index.row();
    iFoxtrotLight *light = dynamic_cast<iFoxtrotLight *>(session.getModel()->at(row));
    light->click();
    bool onOff = !light->getOnOff();
    QByteArray req = light->GTSAP("SET", "ONOFF", onOff ? "1" : "0");
    light->setOnOff(onOff);
    emit session.getModel()->dataChanged(index, index);
    ui->labelLightStatus->setText(onOff ? "1" : "0");
    qDebug() << "REQ" << req;
    session.write(req);
}

void MainWindow::on_pushButtonRelay_clicked()
{
    QModelIndex index = ui->listViewItems->currentIndex();
    int row = index.row();
    iFoxtrotRelay *relay = dynamic_cast<iFoxtrotRelay *>(session.getModel()->at(row));
    relay->click();
    bool onOff = !relay->getOnOff();
    QByteArray req = relay->GTSAP("SET", "ONOFF", onOff ? "1" : "0");
    relay->setOnOff(onOff);
    emit session.getModel()->dataChanged(index, index);
    ui->labelRelayStatus->setText(onOff ? "1" : "0");
    qDebug() << "REQ" << req;
    session.write(req);
}

void MainWindow::buttonSceneClicked()
{
    const QPushButton *s = dynamic_cast<QPushButton *>(sender());
    if (!s) {
        qDebug() << "bad sender" << sender();
        return;
    }
    QString set = s->objectName();
    set.replace(0, set.size() - 1, "SET");

    int row = ui->listViewItems->currentIndex().row();
    iFoxtrotScene *scene = dynamic_cast<iFoxtrotScene *>(session.getModel()->at(row));
    QByteArray req = scene->GTSAP("SET", set, "1");
    qDebug() << req;
    session.write(req);
}
