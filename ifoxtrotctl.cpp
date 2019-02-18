#include <QDebug>

#include "ifoxtrotctl.h"
#include "ui_mainwindow.h"

iFoxtrotCtl *iFoxtrotCtl::getOne(const QString &foxType, const QString &foxName)
{
    if (foxType == "LIGHT")
        return new iFoxtrotLight(foxName);
    if (foxType == "RELAY")
        return new iFoxtrotRelay(foxName);

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

    if (iFoxtrotCtl::setProp(prop, val))
        return true;

    return false;
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

    if (iFoxtrotOnOff::setProp(prop, val))
        return true;

    return false;
}

void iFoxtrotLight::setupUI(Ui::MainWindow *ui){
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

    if (iFoxtrotOnOff::setProp(prop, val))
        return true;

    return false;
}

void iFoxtrotRelay::setupUI(Ui::MainWindow *ui){
    ui->labelRelayStatus->setText(onOff ? "1" : "0");
}
