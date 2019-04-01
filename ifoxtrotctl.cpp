#include <QDebug>
#include <QList>

#include "ifoxtrotctl.h"
#include "ifoxtrotsession.h"
#include "ui_mainwindow.h"

iFoxtrotCtl *iFoxtrotCtl::getOne(iFoxtrotSession *session,
                                 const QString &foxType, const QString &foxName)
{
    if (foxType == "DISPLAY")
        return new iFoxtrotDisplay(session, foxName);
    if (foxType == "LIGHT")
        return new iFoxtrotLight(session, foxName);
    if (foxType == "RELAY")
        return new iFoxtrotRelay(session, foxName);
    if (foxType == "SCENE")
        return new iFoxtrotScene(session, foxName);
    if (foxType == "SHUTTER")
        return new iFoxtrotShutter(session, foxName);

    return nullptr;
}

bool iFoxtrotCtl::setProp(const QString &prop, const QString &val)
{
    if (prop == "ENABLE")
        return true;

    if (prop == "NAME") {
        if (!val.startsWith('"') || !val.endsWith('"')) {
            qWarning() << "wrong name for" << foxName << ":" << val;
            return false;
        }
        name = val;
        name.remove(0, 1);
        name.chop(1);

        return true;
    }

    qWarning() << "unknown property" << prop << "for" << getFoxType();

    return false;
}

QByteArray iFoxtrotCtl::GTSAP(const QString &prefix, const QString &prop,
                              const QString &set) const
{
    QByteArray ret;

    ret.append(prefix).append(':').append(getFoxName()).append(".GTSAP1_").
            append(getFoxType()).append('_').append(prop);

    if (!set.isEmpty())
        ret.append(',').append(set);

    ret.append('\n');

    return ret;
}

QVariant iFoxtrotCtl::data(int column, int role) const
{
    if (column > 1)
        qDebug() << __PRETTY_FUNCTION__ << column;

    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return getFoxType().mid(0, 1) + ' ' + getName();

    if (role == Qt::BackgroundRole)
        return QBrush(getColor());

    return QVariant();
}

void iFoxtrotCtl::changed(const QString &prop)
{
    Q_UNUSED(prop);
    session->getModel()->changed(this);
}

void iFoxtrotOnOff::switchState()
{
    QByteArray req = GTSAP("SET", "ONOFF", onOff ? "0" : "1");
    qDebug() << "REQ" << req;
    session->write(req);
}

bool iFoxtrotOnOff::setProp(const QString &prop, const QString &val)
{
    if (prop == "ONOFF") {
        onOff = (val == "1");
        changed(prop);
        return true;
    }

    return iFoxtrotCtl::setProp(prop, val);
}

QVariant iFoxtrotOnOff::data(int column, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
        case 1:
            return onOff ? "1" : "0";
        }
    }

    if (role == Qt::FontRole && onOff) {
        QFont f;
        f.setWeight(QFont::Bold);
        return f;
    }

    return iFoxtrotCtl::data(column, role);
}

bool iFoxtrotLight::setProp(const QString &prop, const QString &val)
{
    if (prop == "TYPE") {
        dimmable = (val == "1");
        return true;
    }
    if (prop == "DIMTYPE") {
        rgb = (val == "1");
        return true;
    }
    if (prop == "DIMLEVEL") {
        dimlevel = val.toFloat();
        changed(prop);
        return true;
    }

    return iFoxtrotOnOff::setProp(prop, val);
}

void iFoxtrotLight::setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper)
{
    //ui->labelLightStatus->setText(onOff ? "1" : "0");
    ui->horizontalSliderDimlevel->setVisible(dimmable);
    ui->horizontalSliderDimlevel->setValue((int)dimlevel);
    widgetMapper.addMapping(ui->labelLightStatus, 1, "text");
    widgetMapper.addMapping(ui->horizontalSliderDimlevel, 2, "value");
}

QVariant iFoxtrotLight::data(int column, int role) const
{
    if (column > 0)
        qDebug() << __func__ << column << role;

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
        case 2:
            return (int)dimlevel;
        }
    }

    return iFoxtrotOnOff::data(column, role);
}

bool iFoxtrotRelay::setProp(const QString &prop, const QString &val)
{
    if (prop == "SYMBOL") {
        return true;
    }
    if (prop == "TYPE") {
        return true;
    }

    return iFoxtrotOnOff::setProp(prop, val);
}

void iFoxtrotRelay::setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper)
{
    widgetMapper.addMapping(ui->labelRelayStatus, 1, "text");
}

bool iFoxtrotDisplay::setProp(const QString &prop, const QString &val)
{
    if (prop == "EDIT") {
        editable = (val == "1");
        return true;
    }
    if (prop == "TYPE") {
        real = (val == "1");
        return true;
    }
    if (prop == "SYMBOL") {
        return true;
    }
    if (prop == "VALUE") {
        value = val.toDouble();
        changed(prop);
        return true;
    }
    if (prop == "UNIT") {
        unit = val;
        unit.remove(0, 1);
        unit.chop(1);
        return true;
    }
    if (prop == "PRECISION") {
        precision = val.toInt();
        return true;
    }
    if (prop == "URL") {
        return true;
    }
    if (prop == "INCVALUE") {
        return true;
    }
    if (prop == "DECVALUE") {
        return true;
    }
    if (prop == "VALUESET") {
        return true;
    }

    if (iFoxtrotCtl::setProp(prop, val))
        return true;

    return false;
}

void iFoxtrotDisplay::setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper)
{
    ui->doubleSpinBoxDisplayVal->setReadOnly(!editable);
    ui->doubleSpinBoxDisplayVal->setSuffix(" " + unit);
    widgetMapper.addMapping(ui->doubleSpinBoxDisplayVal, 1, "value");
}

QVariant iFoxtrotDisplay::data(int column, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
        case 1:
            return value;
        }
    }

    return iFoxtrotCtl::data(column, role);
}

bool iFoxtrotShutter::setProp(const QString &prop, const QString &val)
{
    if (prop == "UP") {
        status = MovingUp;
        changed(prop);
        return true;
    }
    if (prop == "DOWN") {
        status = MovingDown;
        changed(prop);
        return true;
    }
    if (prop == "RUN") {
        status = Moving;
        changed(prop);
        return true;
    }
    if (prop == "UPPOS") {
        status = UpPos;
        changed(prop);
        return true;
    }
    if (prop == "DOWNPOS") {
        status = DownPos;
        changed(prop);
        return true;
    }
    if (prop == "UP_CONTROL" ||
            prop == "DOWN_CONTROL" ||
            prop == "ROTUP_CONTROL" ||
            prop == "ROTDOWN_CONTROL")
        return true;

    return iFoxtrotCtl::setProp(prop, val);
}

void iFoxtrotShutter::setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper)
{
    widgetMapper.addMapping(ui->labelShutterStatus, 1, "text");
}

QVariant iFoxtrotShutter::data(int column, int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        switch (column) {
        case 1:
            return QString("not moving");
        }
    }

    return iFoxtrotCtl::data(column, role);
}

void iFoxtrotShutter::up()
{
    QByteArray req = GTSAP("SET", "UP_CONTROL", "1");
    qDebug() << req;
    session->write(req);
}

void iFoxtrotShutter::down()
{
    QByteArray req = GTSAP("SET", "DOWN_CONTROL", "1");
    qDebug() << req;
    session->write(req);
}

void iFoxtrotShutter::rotUp()
{
    QByteArray req = GTSAP("SET", "ROTUP_CONTROL", "1");
    qDebug() << req;
    session->write(req);
}

void iFoxtrotShutter::rotDown()
{
    QByteArray req = GTSAP("SET", "ROTDOWN_CONTROL", "1");
    qDebug() << req;
    session->write(req);
}

bool iFoxtrotScene::setProp(const QString &prop, const QString &val)
{
    if (prop == "FILE") {
        return true;
    }
    if (prop == "NUM") {
        bool ok;
        scenes = val.toInt(&ok);
        if (!ok || (scenes != 4 && scenes != 8))
            qWarning() << "invalid scene NUM" << val;
        return true;
    }
    if (prop.length() == 4 && prop.startsWith("SET") && prop.at(3).isDigit()) {
        return true;
    }

    return iFoxtrotCtl::setProp(prop, val);
}

void iFoxtrotScene::setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper)
{
    Q_UNUSED(widgetMapper);

    for (QPushButton *but : ui->page_SCENE->findChildren<QPushButton *>(QString(),
                                    Qt::FindDirectChildrenOnly)) {
        QString name = but->objectName();

        if (!name.startsWith("pushButtonSc"))
            continue;

        int which = name.remove(0, sizeof "pushButtonSc" - 1).toInt();
        but->setEnabled(which <= scenes);
    }
}
