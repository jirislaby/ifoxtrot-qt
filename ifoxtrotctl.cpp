#include <QDebug>

#include "ifoxtrotctl.h"
#include "ui_mainwindow.h"

iFoxtrotCtl *iFoxtrotCtl::getOne(const QString &foxType, const QString &foxName)
{
    if (foxType == "LIGHT")
        return new iFoxtrotLight(foxName);
    if (foxType == "RELAY")
        return new iFoxtrotRelay(foxName);
    if (foxType == "DISPLAY")
        return new iFoxtrotDisplay(foxName);
    if (foxType == "SHUTTER")
        return new iFoxtrotShutter(foxName);

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

    if (iFoxtrotCtl::setProp(prop, val))
        return true;

    return false;
}

void iFoxtrotShutter::setupUI(Ui::MainWindow *ui)
{
}
