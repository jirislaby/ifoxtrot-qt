#ifndef IFOXTROTCTL_H
#define IFOXTROTCTL_H

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

    QString getFoxName() { return foxName; }
    virtual QString getFoxType() = 0;
    QString getName() { return name; }

    virtual void setupUI(Ui::MainWindow *ui) = 0;
    virtual bool setProp(const QString &prop, const QString &val);

    static iFoxtrotCtl *getOne(const QString &foxType, const QString &foxName);
protected:
    QString foxName;
    QString name;
};

class iFoxtrotOnOff : public iFoxtrotCtl {
public:
    iFoxtrotOnOff(const QString &foxName) :
            iFoxtrotCtl(foxName), onOff(false) {}
    virtual ~iFoxtrotOnOff() = default;

    bool getOnOff() { return onOff; }
    void setOnOff(bool onOff) { this->onOff = onOff; }

    virtual bool setProp(const QString &prop, const QString &val);

protected:
    bool onOff;
};

class iFoxtrotLight : public iFoxtrotOnOff {
public:
    iFoxtrotLight(const QString &foxName) : iFoxtrotOnOff(foxName), dimmable(false), rgb(false) { }

    QString getFoxType() override { return "LIGHT"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui) override;

private:
    bool dimmable;
    bool rgb;
    float dimlevel;
};

class iFoxtrotRelay : public iFoxtrotOnOff {
public:
    iFoxtrotRelay(const QString &foxName) : iFoxtrotOnOff(foxName) { }

    QString getFoxType() override { return "RELAY"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui) override;

private:
};

#endif // IFOXTROTCTL_H
