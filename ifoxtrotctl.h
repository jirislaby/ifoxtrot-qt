#ifndef IFOXTROTCTL_H
#define IFOXTROTCTL_H

#include <QColor>
#include <QObject>
#include <QString>

namespace Ui {
    class MainWindow;
};

class iFoxtrotCtl : public QObject
{
    Q_OBJECT
public:
    iFoxtrotCtl(const QString &foxName) : foxName(foxName), name("") { }
    virtual ~iFoxtrotCtl() = default;

    QString getFoxName() const { return foxName; }
    virtual QString getFoxType() const = 0;
    QString getName() const { return name; }

    virtual void setupUI(Ui::MainWindow *ui) = 0;
    virtual bool setProp(const QString &prop, const QString &val);

    /* ui */
    virtual QColor getColor() const { return Qt::white; }
    virtual bool isBold() const { return false; }

    static iFoxtrotCtl *getOne(const QString &foxType, const QString &foxName);
protected:
    QString foxName;
    QString name;
};

class iFoxtrotOnOff : public iFoxtrotCtl {
public:
    iFoxtrotOnOff(const QString &foxName) :
            iFoxtrotCtl(foxName), onOff(false) {}
    virtual ~iFoxtrotOnOff() override = default;

    bool getOnOff() const { return onOff; }
    void setOnOff(bool onOff) { this->onOff = onOff; }

    virtual bool setProp(const QString &prop, const QString &val) override;

    virtual bool isBold() const override { return onOff; }

protected:
    bool onOff;
};

class iFoxtrotLight : public iFoxtrotOnOff {
public:
    iFoxtrotLight(const QString &foxName) : iFoxtrotOnOff(foxName), dimmable(false), rgb(false) { }

    QString getFoxType() const override { return "LIGHT"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui) override;

    QColor getColor() const override { return QColor(250, 250, 210); }
private:
    bool dimmable;
    bool rgb;
    float dimlevel;
};

class iFoxtrotRelay : public iFoxtrotOnOff {
public:
    iFoxtrotRelay(const QString &foxName) : iFoxtrotOnOff(foxName) { }

    QString getFoxType() const override { return "RELAY"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui) override;

    QColor getColor() const override { return QColor(250, 250, 250); }
private:
};

#endif // IFOXTROTCTL_H