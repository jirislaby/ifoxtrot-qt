#ifndef IFOXTROTCTL_H
#define IFOXTROTCTL_H

#include <QByteArray>
#include <QColor>
#include <QDataWidgetMapper>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPair>
#include <QString>
#include <QVector>

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
    bool setName(const QString &name);
    QString getName() const { return name; }

    virtual void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) = 0;
    virtual bool setProp(const QString &prop, const QString &val);

    virtual void postReceive() {}

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
    static QString getFoxString(const QString &val);
};

class iFoxtrotOnOff : public iFoxtrotCtl {
    Q_OBJECT
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
    Q_OBJECT
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
    Q_OBJECT
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

class iFoxtrotSocket : public iFoxtrotOnOff {
    Q_OBJECT
public:
    iFoxtrotSocket(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotOnOff(session, foxName) { }

    QString getFoxType() const override { return "SOCKET"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) override;

protected:
    QColor getColor() const override { return QColor(210, 210, 250); }

private:
};

class iFoxtrotDisplay : public iFoxtrotCtl {
    Q_OBJECT
public:
    iFoxtrotDisplay(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotCtl(session, foxName), editable(false), real(false), value(0),
        precision(0), unit("") {}

    QString getFoxType() const override { return "DISPLAY"; }
    double getValue() const { return value; }
    double getPrecision() const { return precision; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) override;

    QVariant data(int column, int role) const override;

protected:
    QColor getColor() const override { return QColor(210, 210, 255); }

private:
    bool editable;
    bool real;
    double value;
    int precision;
    QString unit;
};

class iFoxtrotShutter : public iFoxtrotCtl {
    Q_OBJECT
public:
    enum ShutterStatus {
        Steady,
        MovingUp,
        MovingDown,
    };

    iFoxtrotShutter(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotCtl(session, foxName), status(Steady), upPos(false),
        downPos(false), running(false) {}

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
    bool upPos;
    bool downPos;
    bool running;

    QString stringStatus() const;
    QString stringPosition() const;
};

class iFoxtrotPIRSENSOR : public iFoxtrotCtl {
    Q_OBJECT
public:
    iFoxtrotPIRSENSOR(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotCtl(session, foxName), value(0) {}

    QString getFoxType() const override { return "PIRSENSOR"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) override;

    QVariant data(int column, int role) const override;

protected:
    QColor getColor() const override { return QColor(100, 210, 210); }

private:
    int value;
};

class iFoxtrotScene : public iFoxtrotCtl {
    Q_OBJECT
public:
    class SceneCfg {
    public:
        using Member = QPair<iFoxtrotCtl *, QPair<QString, QString>>;

        SceneCfg() : lights(0), relays(0), shutters(0), others(0) {};

        void add(iFoxtrotCtl *ctl, QString gtsap, QString val) {
            if (dynamic_cast<iFoxtrotLight *>(ctl))
                lights++;
            else if (dynamic_cast<iFoxtrotRelay *>(ctl))
                relays++;
            else if (dynamic_cast<iFoxtrotShutter *>(ctl))
                shutters++;
            else
                others++;
            members.append(Member(ctl, Member::second_type(gtsap, val)));
        }

        QList<Member> getMembers() const { return members; }

        int getLights() const { return lights; }
        int getRelays() const { return relays; }
        int getShutters() const { return shutters; }
        int getOthers() const { return others; }
    private:
        QList<Member> members;
        int lights, relays, shutters, others;
    };

    iFoxtrotScene(iFoxtrotSession *session, const QString &foxName);

    QString getFoxType() const override { return "SCENE"; }
    bool setProp(const QString &prop, const QString &val) override;
    void postReceive() override;

    void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) override;

    QVariant data(int column, int role) const override;

protected:
    QColor getColor() const override { return QColor(210, 255, 210); }

private:
    QVector<QString> sceneNames;
    int scenes;
    QString filename;
    QVector<SceneCfg> sceneCfg;

    void walkSceneDFS(const int number, const QJsonObject &scene,
                      QString prefix = "");
};

class iFoxtrotTPW : public iFoxtrotCtl {
    Q_OBJECT
public:
    iFoxtrotTPW(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotCtl(session, foxName) {}

    QString getFoxType() const override { return "TPW"; }
    double getDelta() const { return delta; }
    bool setProp(const QString &prop, const QString &val) override;
    void postReceive() override;

    void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) override;

    QVariant data(int column, int role) const override;

protected:
    QColor getColor() const override { return QColor(210, 255, 255); }

private:
    unsigned short type;
    QString name;
    QString file;
    quint32 crc;
    bool manual;
    bool holiday;
    bool heat;
    bool cool;
    short mode;
    double roomTemp;
    double heatTemp;
    double coolTemp;
    double delta;
};

class iFoxtrotWebCam : public iFoxtrotCtl {
    Q_OBJECT
public:
    iFoxtrotWebCam(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotCtl(session, foxName) {}

    QString getFoxType() const override { return "WEBCAM"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) override;

    QVariant data(int column, int role) const override;

protected:
    QColor getColor() const override { return QColor(230, 155, 155); }

private:
    QString url;
};

class iFoxtrotWebConf : public iFoxtrotCtl {
    Q_OBJECT
public:
    iFoxtrotWebConf(iFoxtrotSession *session, const QString &foxName) :
        iFoxtrotCtl(session, foxName) {}

    QString getFoxType() const override { return "WEBCONF"; }
    bool setProp(const QString &prop, const QString &val) override;

    void setupUI(Ui::MainWindow *ui, QDataWidgetMapper &widgetMapper) override;

    QVariant data(int column, int role) const override;

protected:
    QColor getColor() const override { return QColor(210, 155, 255); }

private:
    QString url;
};

#endif // IFOXTROTCTL_H
