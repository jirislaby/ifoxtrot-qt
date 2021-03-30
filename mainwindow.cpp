#include <QMessageBox>
#include <QScreen>
#include <QSettings>

#include <QtGlobal>

#include "filetransfer.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    changingVals(false)
{
    QSettings settings("jirislaby", "ifoxtrot");

    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    // settings
    settings.beginGroup("mainwindow");
    if (settings.contains("size") && settings.contains("pos")) {
	    auto size = settings.value("size").toSize();
	    auto pos = settings.value("pos").toPoint();

	    for (auto scr : QGuiApplication::screens()) {
		    if (scr->geometry().contains(pos)) {
			    resize(size);
			    move(pos);
			    break;
		    }
	    }
    }
    settings.endGroup();

    ui->lineEditPLCAddr->setText(settings.value("PLCaddr").toString());
    ui->lineEditAddr->setText(settings.value("IPaddr").toString());
    ui->spinBoxPort->setValue(settings.value("port", "5010").toInt());
    ui->checkBoxAutocon->setChecked(settings.value("autoconnect", false).toBool());

    connect(&session, &iFoxtrotSession::received, [this] {
	    proxyModel->sort(0);
    });
    connect(&session, &iFoxtrotSession::connected, this, &MainWindow::connected);
    connect(&session, &iFoxtrotSession::conStatusUpdate, this, &MainWindow::conStatusUpdate);
    connect(&session, &iFoxtrotSession::disconnected, this, &MainWindow::disconnected);
    connect(&session, &iFoxtrotSession::sockError, this, &MainWindow::sockError);
    connect(&session, &iFoxtrotSession::conError, this, &MainWindow::conError);

    proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(session.getModel());
    proxyModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
    ui->listViewItems->setModel(proxyModel);
    connect(ui->listViewItems->selectionModel(),
            &QItemSelectionModel::currentRowChanged, this,
            &MainWindow::rowChanged);

    ui->butDisconnect->setVisible(false);

    if (ui->checkBoxAutocon->isChecked())
        emit ui->butConnect->click();

    widgetMapper.setModel(session.getModel());
}

MainWindow::~MainWindow()
{
    QSettings settings("jirislaby", "ifoxtrot");

    session.close();

    settings.beginGroup("mainwindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.endGroup();

    settings.setValue("PLCaddr", ui->lineEditPLCAddr->text());
    settings.setValue("IPaddr", ui->lineEditAddr->text());
    settings.setValue("port", ui->spinBoxPort->value());
    settings.setValue("autoconnect", ui->checkBoxAutocon->isChecked());

    delete ui;
}

void MainWindow::on_butConnect_clicked()
{
    QString addr = ui->lineEditAddr->text();
    QString PLCAddr = ui->lineEditPLCAddr->text();
    quint16 port = static_cast<quint16>(ui->spinBoxPort->value());

    if (addr.isEmpty()) {
        QMessageBox mbox;
        mbox.setIcon(QMessageBox::Critical);
	mbox.setText(tr("Empty address!"));
        mbox.exec();
        return;
    }

    session.connectToHost(addr, port, PLCAddr);

    statusBar()->showMessage(tr("Connecting to ") + addr + ":" + QString::number(port));
    ui->butConnect->setVisible(false);
    ui->butDisconnect->setVisible(true);
}

void MainWindow::on_butDisconnect_clicked()
{
    switch (session.getState()) {
    case iFoxtrotSession::Connecting:
        session.abort();
        disconnected();
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
	proxyModel->sort(0);
	statusBar()->showMessage(tr("Connected to ") + session.getPeerName());
	ui->labelPLC->setText(session.getPLCVersion());
}

void MainWindow::conStatusUpdate(const QString &status)
{
    statusBar()->showMessage(session.getPeerName() + ": " + status);
}

void MainWindow::disconnected()
{
    statusBar()->showMessage(tr("Disconnected"));
    ui->butConnect->setVisible(true);
    ui->butDisconnect->setVisible(false);
    ui->labelFoxName->setText("");
    ui->labelPLC->setText("");
    ui->stackedWidget->setCurrentIndex(0);
}

void MainWindow::conError(const QString &reason)
{
	qWarning() << "connection error" << reason;
	disconnected();
	statusBar()->showMessage(tr("Connection error: ") + reason);
}

void MainWindow::sockError(QAbstractSocket::SocketError socketError)
{
    qWarning() << "disconnected" << socketError;
    disconnected();
    statusBar()->showMessage(tr("Socket error: ") + QString::number(socketError));
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

    const QModelIndex proxyIndex = proxyModel->mapToSource(index);
    if (!proxyIndex.isValid())
        return;

    iFoxtrotCtl *item = session.getModel()->at(proxyIndex.row());
    QString name = item->getName();

    for (int i = 0; i < ui->stackedWidget->count(); i++)
        if (ui->stackedWidget->widget(i)->objectName().mid(5) == item->getFoxType()) {
            ui->stackedWidget->setCurrentIndex(i);
            break;
        }

    ui->labelFoxName->setText(name);
    changingVals = true;
    widgetMapper.clearMapping();
    item->setupUI(ui, widgetMapper);
    widgetMapper.setCurrentIndex(proxyIndex.row());
    changingVals = false;
}

void MainWindow::on_listViewItems_doubleClicked(const QModelIndex &index)
{
    const QModelIndex proxyIndex = proxyModel->mapToSource(index);
    iFoxtrotCtl *item = session.getModel()->at(proxyIndex.row());
    item->doubleClick();
}

void MainWindow::on_pushButtonLight_clicked()
{
    auto ctl = getCurrentCtl();
    auto onOff = dynamic_cast<iFoxtrotOnOff *>(ctl);
    if (!onOff) {
        qWarning() << "invalid ctl" << __LINE__ << ctl->getFoxType() <<
                      ctl->getFoxName();
        return;
    }
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

    auto ctl = getCurrentCtl();
    auto scene = dynamic_cast<iFoxtrotScene *>(ctl);
    if (!scene) {
        qWarning() << "invalid ctl" << __LINE__ << ctl->getFoxType() <<
                      ctl->getFoxName();
        return;
    }
    QByteArray req = scene->GTSAP("SET", set, "1");
    qDebug() << req;
    session.write(req);
}

void MainWindow::fileTransferTriggered()
{
    FileTransfer ft(&session, this);

    ft.exec();
}

void MainWindow::aboutTriggered()
{
	QMessageBox::about(this, "iFoxtrot", tr(
			   "Copyright © 2018-2021 <b>Jiri Slaby</b><br/>"
	                   "Licensed under GPLv2<br/>"
	                   "Web: <a href=\"http://consultctl.eu\">http://consultctl.eu</a><br/>"
			   "E-mail: <a href=\"mailto:jirislaby@gmail.com\">jirislaby@gmail.com</a>"));
}

void MainWindow::aboutQtTriggered()
{
	QMessageBox::aboutQt(this);
}

void MainWindow::on_pushButtonShutUp_clicked()
{
    auto ctl = getCurrentCtl();
    auto shutter = dynamic_cast<iFoxtrotShutter *>(ctl);
    if (!shutter) {
        qWarning() << "invalid ctl" << __LINE__ << ctl->getFoxType() <<
                      ctl->getFoxName();
        return;
    }
    shutter->up();
}

void MainWindow::on_pushButtonShutRUp_clicked()
{
    auto ctl = getCurrentCtl();
    auto shutter = dynamic_cast<iFoxtrotShutter *>(ctl);
    if (!shutter) {
        qWarning() << "invalid ctl" << __LINE__ << ctl->getFoxType() <<
                      ctl->getFoxName();
        return;
    }
    shutter->rotUp();
}

void MainWindow::on_pushButtonShutRDown_clicked()
{
    auto ctl = getCurrentCtl();
    auto shutter = dynamic_cast<iFoxtrotShutter *>(ctl);
    if (!shutter) {
        qWarning() << "invalid ctl" << __LINE__ << ctl->getFoxType() <<
                      ctl->getFoxName();
        return;
    }
    shutter->rotDown();
}

void MainWindow::on_pushButtonShutDown_clicked()
{
    auto ctl = getCurrentCtl();
    auto shutter = dynamic_cast<iFoxtrotShutter *>(ctl);
    if (!shutter) {
        qWarning() << "invalid ctl" << __LINE__ << ctl->getFoxType() <<
                      ctl->getFoxName();
        return;
    }
    shutter->down();
}

void MainWindow::on_lineEditFilter_textEdited(const QString &text)
{
    proxyModel->setFilterFixedString(text);
}

iFoxtrotCtl *MainWindow::getCurrentCtl() const
{
    const auto curIdx = ui->listViewItems->currentIndex();
    if (!curIdx.isValid()) {
        qWarning() << "invalid current index";
        return nullptr;
    }

    const auto idx = proxyModel->mapToSource(curIdx);
    if (!idx.isValid()) {
        qWarning() << "invalid proxy index";
        return nullptr;
    }

    return session.getModel()->at(idx.row());
}

void MainWindow::on_horizontalSliderDimlevel_sliderReleased()
{
    int value = ui->horizontalSliderDimlevel->value();
    auto ctl = getCurrentCtl();
    auto light = dynamic_cast<iFoxtrotLight *>(ctl);
    if (!light) {
        qWarning() << "invalid ctl" << __LINE__ << ctl->getFoxType() <<
                      ctl->getFoxName();
        return;
    }
    QByteArray req = light->GTSAP("SET", "DIMLEVEL", QString::number(value));
    qDebug() << req;
    session.write(req);
}

void MainWindow::on_horizontalSliderDimlevel_actionTriggered(int action)
{
    if (action >= 1 && action <= 6)
        on_horizontalSliderDimlevel_sliderReleased();
}

void MainWindow::on_doubleSpinBoxDisplayVal_valueChanged(double value)
{
    auto ctl = getCurrentCtl();
    auto display = dynamic_cast<iFoxtrotDisplay *>(ctl);
    if (!display) {
        qWarning() << "invalid ctl" << __LINE__ << ctl->getFoxType() <<
                      ctl->getFoxName();
        return;
    }

    if (changingVals)
	    return;

    if (abs(display->getValue() - value) < .01)
        return;

    QByteArray req = display->GTSAP("SET", "VALUESET", QString::number(value));
    qDebug() << req;
    session.write(req);
}

void MainWindow::on_TPW_SB_delta_valueChanged(double value)
{
    auto ctl = getCurrentCtl();
    auto tpw = dynamic_cast<iFoxtrotTPW *>(ctl);
    if (!tpw) {
        qWarning() << "invalid ctl" << __LINE__ << ctl->getFoxType() <<
                      ctl->getFoxName();
        return;
    }
    double diff = value - tpw->getDelta();

    if (changingVals)
	    return;

    if (abs(diff) < .01)
        return;

    /* there is no setdelta... */
    QByteArray req = tpw->GTSAP("SET",
                                diff > 0 ? "INCDELTA" : "DECDELTA", "1");
    qDebug() << req;
    session.write(req);
}
