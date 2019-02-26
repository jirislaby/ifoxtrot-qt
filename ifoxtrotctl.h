#ifndef IFOXTROTCTL_H
#define IFOXTROTCTL_H

#include <QByteArray>
#include <QColor>
#include <QObject>
#include <QString>

class iFoxtrotSession;

namespace Ui {
    class MainWindow;
};

class iFoxtrotCtl : public QObject
{
    Q_OBJECT
public:
    iFoxtrotCtl(iFoxtrotSession *session, const QString &foxName) :
        session(session), foxName(foxName), name("") { }
    virtual ~iFoxtrotCtl() = default;

    QString getFoxName() const { return foxName; }
    virtual QString getFoxType() const = 0;
    QString getName() const { return name; }

    virtual void setupUI(Ui::MainWindow *ui) = 0;
    virtual bool setProp(const QString &prop, const QString &val);

    QByteArray GTSAP(const QString &prefix, const QString &prop,
                     const QString &set = QString()) const;

    /* ui */
    virtual QColor getColor() const { return Qt::white; }
    virtual bool isBold() const { return false; }

    virtual void click() {}

    static iFoxtrotCtl *getOne(iFoxtrotSession *session,
                               const QString &foxType, const QString &foxName);
protected:
    iFoxtrotSession *session;
    QString foxName;
    QString name;
};

class iFoxtrotOnOff : public iFoxtrotCtl {
public:
    iFoxtrotOnOff(iFoxtrotSession *session, const QString &foxName) :
            iFoxtrotCtl(session, foxName), onOff(false) {}
    virtual ~iFoxtrotOnOff() override = default;

    bool getOnOff() const { return onOff; }
    void setOnOff(bool onOff) { this->onOff = onOff; }

    virtual bool setProp(const QString &prop, const QString &val) override;

    virtual bool isBold() const override { return onOff; }

    virtual void click() override;

protected:
    bool onOff;
};

class iFoxtrotLight : public iFoxtrotOnOff {
public:
    iFoxtrotLight(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotOnOff(session, foxName), dimmable(false), rgb(false),
        dimlevel(0) { }

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
    iFoxtrotRelay(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotOnOff(session, foxName) { }

    QString getFoxType() const override { return "RELAY"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui) override;

    QColor getColor() const override { return QColor(250, 250, 250); }
private:
};

class iFoxtrotDisplay : public iFoxtrotCtl {
public:
    iFoxtrotDisplay(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotCtl(session, foxName), editable(false), real(false), value(0),
        unit("") {}

    /*bool getOnOff() const { return onOff; }
    void setOnOff(bool onOff) { this->onOff = onOff; }*/

    QString getFoxType() const override { return "DISPLAY"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui) override;

    QColor getColor() const override { return QColor(210, 210, 255); }
private:
    bool editable;
    bool real;
    double value;
    QString unit;
};

class iFoxtrotShutter : public iFoxtrotCtl {
public:
    iFoxtrotShutter(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotCtl(session, foxName) {}

    /*bool getOnOff() const { return onOff; }
    void setOnOff(bool onOff) { this->onOff = onOff; }*/

    QString getFoxType() const override { return "SHUTTER"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui) override;

    QColor getColor() const override { return QColor(255, 210, 210); }
private:
};

class iFoxtrotScene : public iFoxtrotCtl {
public:
    iFoxtrotScene(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotCtl(session, foxName), scenes(0) {}

    QString getFoxType() const override { return "SCENE"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui) override;

    QColor getColor() const override { return QColor(210, 255, 210); }
private:
    int scenes;
};

#endif // IFOXTROTCTL_H
