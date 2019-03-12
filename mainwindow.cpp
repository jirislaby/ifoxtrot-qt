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

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(session.getModel());
    ui->listViewItems->setModel(proxyModel);
    connect(ui->listViewItems->selectionModel(),
            &QItemSelectionModel::currentRowChanged, this,
            &MainWindow::rowChanged);

    ui->butDisconnect->setVisible(false);

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
    ui->butConnect->setVisible(false);
    ui->butDisconnect->setVisible(true);
}

void MainWindow::on_butDisconnect_clicked()
{
    switch (session.getState()) {
    case iFoxtrotSession::Connecting:
        session.abort();
        return;
    case iFoxtrotSession::Connected:
        session.close();
        return;
    default:
        qWarning() << "bad state in disconnect" << session.getState();
        break;
    }
}

void MainWindow::connected()
{
    statusBar()->showMessage("Connected");
    ui->labelPLC->setText(session.getPLCVersion());
    ui->listViewItems->setFocus();
}

void MainWindow::disconnected()
{
    statusBar()->showMessage("Disconnected");
    ui->butConnect->setVisible(true);
    ui->butDisconnect->setVisible(false);
    ui->labelFoxName->setText("");
    ui->labelPLC->setText("");
    ui->stackedWidget->setCurrentIndex(0);
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
    if (!index.isValid())
        return;

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
    item->doubleClick();
}

void MainWindow::on_pushButtonLight_clicked()
{
    QModelIndex index = ui->listViewItems->currentIndex();
    auto onOff = dynamic_cast<iFoxtrotOnOff *>(session.getModel()->at(index.row()));
    onOff->switchState();
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

void MainWindow::on_pushButtonShutUp_clicked()
{
    QModelIndex index = ui->listViewItems->currentIndex();
    int row = index.row();
    iFoxtrotShutter *shutter = dynamic_cast<iFoxtrotShutter *>(session.getModel()->at(row));
    shutter->up();
}

void MainWindow::on_pushButtonShutRUp_clicked()
{
    QModelIndex index = ui->listViewItems->currentIndex();
    int row = index.row();
    iFoxtrotShutter *shutter = dynamic_cast<iFoxtrotShutter *>(session.getModel()->at(row));
    shutter->rotUp();
}

void MainWindow::on_pushButtonShutRDown_clicked()
{
    QModelIndex index = ui->listViewItems->currentIndex();
    int row = index.row();
    iFoxtrotShutter *shutter = dynamic_cast<iFoxtrotShutter *>(session.getModel()->at(row));
    shutter->rotDown();
}

void MainWindow::on_pushButtonShutDown_clicked()
{
    QModelIndex index = ui->listViewItems->currentIndex();
    int row = index.row();
    iFoxtrotShutter *shutter = dynamic_cast<iFoxtrotShutter *>(session.getModel()->at(row));
    shutter->down();
}

void MainWindow::on_lineEditFilter_textEdited(const QString &text)
{
    proxyModel->setFilterFixedString(text);
}
