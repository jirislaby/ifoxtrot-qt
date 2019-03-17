#ifndef IFOXTROTCTL_H
#define IFOXTROTCTL_H

#include <QByteArray>
#include <QColor>
#include <QDataWidgetMapper>
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

    virtual void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) = 0;
    virtual bool setProp(const QString &prop, const QString &val);

    QByteArray GTSAP(const QString &prefix, const QString &prop,
                     const QString &set = QString()) const;

    /* ui */
    virtual QVariant data(int column, int role) const;

    virtual void doubleClick() {}

    static iFoxtrotCtl *getOne(iFoxtrotSession *session,
                               const QString &foxType, const QString &foxName);

protected:
    iFoxtrotSession *session;
    QString foxName;
    QString name;

    void changed(const QString &prop);
    virtual QColor getColor() const { return Qt::white; }
};

class iFoxtrotOnOff : public iFoxtrotCtl {
public:
    iFoxtrotOnOff(iFoxtrotSession *session, const QString &foxName) :
            iFoxtrotCtl(session, foxName), onOff(false) {}
    virtual ~iFoxtrotOnOff() override = default;

    bool getOnOff() const { return onOff; }
    void setOnOff(bool onOff) { this->onOff = onOff; }
    virtual void switchState();

    virtual bool setProp(const QString &prop, const QString &val) override;

    virtual QVariant data(int column, int role) const override;

    virtual void doubleClick() override { switchState(); }

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

    void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) override;

    virtual QVariant data(int column, int role) const override;

protected:
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

    void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) override;

protected:
    QColor getColor() const override { return QColor(250, 250, 250); }

private:
};

class iFoxtrotDisplay : public iFoxtrotCtl {
public:
    iFoxtrotDisplay(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotCtl(session, foxName), editable(false), real(false), value(0),
        unit("") {}

    QString getFoxType() const override { return "DISPLAY"; }
    double getValue() const { return value; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) override;

    QVariant data(int column, int role) const override;

protected:
    QColor getColor() const override { return QColor(210, 210, 255); }

private:
    bool editable;
    bool real;
    double value;
    QString unit;
};

class iFoxtrotShutter : public iFoxtrotCtl {
public:
    enum ShutterStatus {
        Steady,
        Moving,
        MovingUp,
        MovingDown,
        UpPos,
        DownPos,
    };

    iFoxtrotShutter(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotCtl(session, foxName), status(Steady) {}

    void up();
    void down();
    void rotUp();
    void rotDown();

    QString getFoxType() const override { return "SHUTTER"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) override;

    QVariant data(int column, int role) const override;

protected:
    QColor getColor() const override { return QColor(255, 210, 210); }

private:
    enum ShutterStatus status;
};

class iFoxtrotScene : public iFoxtrotCtl {
public:
    iFoxtrotScene(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotCtl(session, foxName), scenes(0) {}

    QString getFoxType() const override { return "SCENE"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) override;

protected:
    QColor getColor() const override { return QColor(210, 255, 210); }

private:
    int scenes;
};

#endif // IFOXTROTCTL_H
