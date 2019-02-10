#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringListModel>
#include <QTcpSocket>

namespace Ui {
class MainWindow;
}

class FoxtrotItem {
public:
    /*enum FoxType {
        Unknown,
        Light,
        Relay
    };*/

    FoxtrotItem(QString foxType, QString foxName) : foxType(foxType),foxName(foxName) { }

    //FoxType getType() { return type; }
    QString getFoxType() { return foxType; }
    QString getFoxName() { return foxName; }
    QVariant getProp(QString prop) { return propMap.value(prop); }

    void addProp(QString prop) { propList.append(prop); }
    void setProp(QString prop, QVariant val) { propMap.insert(prop, val); }

    //static FoxType getType(QString);
private:
//    enum FoxType type;
    QString foxType;
    QString foxName;
    QList<QString> propList;
    QMap<QString, QVariant> propMap;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:

    void on_butConnect_clicked();
    void connected();
    void disconnected();
    void readyRead();
    void sockError(QAbstractSocket::SocketError socketError);

private:
    enum ConState {
        Disconnected,
        Connecting,
        Connected
    };
    enum ConState state;
    QTcpSocket socket;
    Ui::MainWindow *ui;
    QStringListModel *model;
    QMap<QString, FoxtrotItem *> items;
};

#endif // MAINWINDOW_H
