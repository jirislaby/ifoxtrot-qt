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

void iFoxtrotOnOff::switchState(const QModelIndex &index)
{
    onOff = !onOff;
    emit session->getModel()->dataChanged(index, index);

    QByteArray req = GTSAP("SET", "ONOFF", onOff ? "1" : "0");
    //ui->labelLightStatus->setText(onOff ? "1" : "0");
    qDebug() << "REQ" << req;
    session->write(req);
}

bool iFoxtrotOnOff::setProp(const QString &prop, const QString &val)
{
    if (prop == "ONOFF") {
        onOff = (val == "1");
        return true;
    }

    return iFoxtrotCtl::setProp(prop, val);
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
        return true;
    }

    return iFoxtrotOnOff::setProp(prop, val);
}

void iFoxtrotLight::setupUI(Ui::MainWindow *ui)
{
    ui->labelLightStatus->setText(onOff ? "1" : "0");
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

void iFoxtrotRelay::setupUI(Ui::MainWindow *ui)
{
    ui->labelRelayStatus->setText(onOff ? "1" : "0");
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
        return true;
    }
    if (prop == "UNIT") {
        unit = val;
        unit.remove(0, 1);
        unit.chop(1);
        return true;
    }
    if (prop == "PRECISION") {
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

void iFoxtrotDisplay::setupUI(Ui::MainWindow *ui)
{
    ui->doubleSpinBoxDisplayVal->setReadOnly(editable);
    ui->doubleSpinBoxDisplayVal->setValue(value);
    ui->labelDisplayUnit->setText(unit);
}

bool iFoxtrotShutter::setProp(const QString &prop, const QString &val)
{
    if (prop == "UP") {
        return true;
    }
    if (prop == "DOWN") {
        return true;
    }
    if (prop == "RUN") {
        return true;
    }
    if (prop == "UPPOS") {
        return true;
    }
    if (prop == "DOWNPOS") {
        return true;
    }
    if (prop == "UP_CONTROL") {
        return true;
    }
    if (prop == "DOWN_CONTROL") {
        return true;
    }
    if (prop == "ROTUP_CONTROL") {
        return true;
    }
    if (prop == "ROTDOWN_CONTROL") {
        return true;
    }

    return iFoxtrotCtl::setProp(prop, val);
}

void iFoxtrotShutter::setupUI(Ui::MainWindow *ui)
{
}

void iFoxtrotShutter::up()
{
}

void iFoxtrotShutter::down()
{
}

void iFoxtrotShutter::rotUp()
{
}

void iFoxtrotShutter::rotDown()
{
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

void iFoxtrotScene::setupUI(Ui::MainWindow *ui)
{
    for (QPushButton *but : ui->page_SCENE->findChildren<QPushButton *>(QString(),
                                    Qt::FindDirectChildrenOnly)) {
        QString name = but->objectName();

        if (!name.startsWith("pushButtonSc"))
            continue;

        int which = name.remove(0, sizeof "pushButtonSc" - 1).toInt();
        but->setEnabled(which <= scenes);
    }
}
